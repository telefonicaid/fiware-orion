/*
*
* Copyright 2023 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <curl/curl.h>                                           // curl

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjParse.h"                                       // kjParse
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjArray, kjObject, kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/curlToBrokerStrerror.h"                 // curlToBrokerStrerror
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/distOp/distOpLookupByCurlHandle.h"             // distOpLookupByCurlHandle
#include "orionld/distOp/distOpSuccess.h"                        // distOpSuccess
#include "orionld/distOp/distOpFailure.h"                        // distOpFailure



// -----------------------------------------------------------------------------
//
// updatedAttr404Purge
//
void updatedAttr404Purge(KjNode* failureV, char* attrName)
{
  if ((failureV == NULL) || (failureV->value.firstChildP == NULL))
    return;

  LM_T(LmtDistOp207, ("********** attribute '%s' is updated - remove 404 failure if present", attrName));

  int     purgedItems = 0;
  KjNode* failureItemP = failureV->value.firstChildP;
  KjNode* next;
  while (failureItemP != NULL)
  {
    next = failureItemP->next;

    KjNode* statusCodeP = kjLookup(failureItemP, "statusCode");
    KjNode* attributeV  = kjLookup(failureItemP, "attributes");

    if ((statusCodeP != NULL)          &&
        (statusCodeP->value.i == 404)  &&
        (attributeV != NULL)           &&
        (kjStringValueLookupInArray(attributeV, attrName) != NULL))
    {
      LM_T(LmtDistOp207, ("Removing '%s' from failureV, as it is present in 'failure' with a 404", attrName));
      kjChildRemove(failureV, failureItemP);
      ++purgedItems;
    }

    failureItemP = next;
  }
  LM_T(LmtDistOp207, ("********** attribute '%s' is updated - %d 404 items were purged", attrName, purgedItems));
}



// -----------------------------------------------------------------------------
//
// entityResponseAccumulate -
//
void entityResponseAccumulate(DistOp* distOpP, KjNode* responseBody, KjNode* successV, KjNode* failureV, int httpResponseCode, CURLMsg* msgP)
{
  //
  // Parse the incoming payload body, if any
  //
  if ((distOpP->rawResponse != NULL) && (distOpP->rawResponse[0] != 0))
  {
    distOpP->responseBody = kjParse(orionldState.kjsonP, distOpP->rawResponse);
    if (distOpP->responseBody == NULL)
      LM_RVE(("Failed to parse the response of a distributed request (Reg '%s')"));
  }
  else
    distOpP->responseBody = NULL;

  if (httpResponseCode == 201)
  {
    if (distOpP->operation != DoCreateEntity)
      LM_W(("Got a 201 response to a forwarded message for distOp operation '%s'", distOpTypes[distOpP->operation]));

    // All the attributes were updated - add them all to "successV"
    for (KjNode* attrP = distOpP->requestBody->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      if (strcmp(attrP->name, "id")   == 0) continue;
      if (strcmp(attrP->name, "type") == 0) continue;

      char* shortName = orionldContextItemAliasLookup(orionldState.contextP, attrP->name, NULL, NULL);

      if (kjStringValueLookupInArray(successV, shortName) == NULL)
      {
        KjNode* nodeP = kjString(orionldState.kjsonP, NULL, shortName);
        kjChildAdd(successV, nodeP);
        LM_T(LmtDistOp207, ("Added attribute '%s' to successV due to a 201 response", shortName));
      }
    }
  }
  else if (httpResponseCode == 204)
  {
    if (distOpP->operation == DoReplaceAttr)
    {
      char*   shortName = orionldContextItemAliasLookup(orionldState.contextP, distOpP->requestBody->value.firstChildP->name, NULL, NULL);
      KjNode* nodeP     = kjString(orionldState.kjsonP, NULL, shortName);

      kjChildAdd(successV, nodeP);
    }
    else if (distOpP->operation == DoDeleteAttrs)
    {
      char*   shortName = orionldContextItemAliasLookup(orionldState.contextP, distOpP->attrName, NULL, NULL);
      KjNode* nodeP     = kjString(orionldState.kjsonP, NULL, shortName);

      kjChildAdd(successV, nodeP);
    }
    else if (distOpP->operation == DoDeleteEntity)
    {
      KjNode* nodeP = kjString(orionldState.kjsonP, NULL, distOpP->entityId);
      kjChildAdd(successV, nodeP);
    }
    else
    {
      // All the attributes were updated - add them all to "successV"
      for (KjNode* attrP = distOpP->requestBody->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        char* shortName = orionldContextItemAliasLookup(orionldState.contextP, attrP->name, NULL, NULL);

        if (kjStringValueLookupInArray(successV, shortName) == NULL)
        {
          KjNode* nodeP = kjString(orionldState.kjsonP, NULL, shortName);
          kjChildAdd(successV, nodeP);
          LM_T(LmtDistOp207, ("Added attribute '%s' to successV due to a 204 response", shortName));
        }
      }
    }
  }
  else if (httpResponseCode == 207)
  {
    if (distOpP->responseBody == NULL)
      LM_RVE(("Reg %s: empty payload body in a 207 response", distOpP->regP->regId));

    if (distOpP->responseBody != NULL)
    {
      KjNode* notUpdatedV = kjLookup(distOpP->responseBody, "notUpdated");
      KjNode* updatedV    = kjLookup(distOpP->responseBody, "updated");

      if ((notUpdatedV != NULL) && (notUpdatedV->value.firstChildP != NULL))
      {
        //
        // The response doesn't include the registrationId, which is needed inside the "failure" objects
        // Of course it isn't !
        // How could the registered endpoint know the registration id ?
        // That is known only by the sender, which is right here.
        //
        // Now, all items in notUpdatedV need to be moved to responseBody::failure
        // BUT, in case of statusCode == 404, we need to look in updatedV.
        // If in updatedV, then we just drop the item altogether.
        //
        // So, we need to go item for item and move to failureV only "non 404-found-in- successV"
        //
        KjNode* notUpdatedP = notUpdatedV->value.firstChildP;
        KjNode* next;

        while (notUpdatedP != NULL)
        {
          next = notUpdatedP->next;

          //
          // if 404, look up the attribute name in successV.
          // If present in successV, then the 404 disappears
          //
          KjNode* statusCodeNodeP = kjLookup(notUpdatedP, "statusCode");
          bool    moveNode        = true;

          if ((statusCodeNodeP != NULL) && (statusCodeNodeP->value.i == 404))
          {
            KjNode* attributesV = kjLookup(notUpdatedP, "attributes");
            if ((attributesV != NULL) && (attributesV->value.firstChildP != NULL))
            {
              char* attributeName = attributesV->value.firstChildP->value.s;
              if (kjStringValueLookupInArray(successV, attributeName) != NULL)
                moveNode = false;
            }
          }

          if (moveNode == false)
          {
            notUpdatedP = next;
            continue;
          }

          //
          // Set the Registration ID
          //
          KjNode* regIdNodeP = kjLookup(notUpdatedP, "registrationId");

          if (regIdNodeP != NULL)
            regIdNodeP->value.s = distOpP->regP->regId;
          else
          {
            regIdNodeP = kjString(orionldState.kjsonP, "registrationId", distOpP->regP->regId);
            kjChildAdd(notUpdatedP, regIdNodeP);
          }

          //
          // Move 'notUpdatedP' from 'notUpdatedV' to 'failureV'
          //
          kjChildRemove(notUpdatedV, notUpdatedP);
          kjChildAdd(failureV, notUpdatedP);

          notUpdatedP = next;
        }
      }

      if (updatedV != NULL)
      {
        //
        // Add to success, if not there already
        // Also, remove from "failure", if present with a 404
        //
        for (KjNode* updatedP = updatedV->value.firstChildP; updatedP != NULL; updatedP = updatedP->next)
        {
          if (kjStringValueLookupInArray(successV, updatedP->value.s) == NULL)
          {
            KjNode* nodeP = kjString(orionldState.kjsonP, NULL, updatedP->value.s);
            kjChildAdd(successV, nodeP);
          }

          updatedAttr404Purge(failureV, updatedP->value.s);
        }
      }
    }
  }
  else if (httpResponseCode == 409)
  {
    //
    // The entire request failed, mark all the attributes accordingly
    //
    distOpFailure(responseBody, distOpP, "Error during Distributed Operation", "Entity already exists", 409, NULL);
  }
  else if (httpResponseCode >= 400)
  {
    //
    // The entire request failed, mark all the attributes accordingly
    //
    KjNode*     titleP  = (distOpP->responseBody != NULL)? kjLookup(distOpP->responseBody, "title")  : NULL;
    KjNode*     detailP = (distOpP->responseBody != NULL)? kjLookup(distOpP->responseBody, "detail") : NULL;
    const char* title   = (titleP  != NULL)? titleP->value.s : "unspecified error from remote provider";
    const char* detail  = (detailP != NULL)? detailP->value.s : NULL;

    if (httpResponseCode == 404)
      orionldState.distOp.e404 += 1;

    distOpFailure(responseBody, distOpP, title, detail, httpResponseCode, NULL);
  }
  else if (httpResponseCode == 0)
  {
    LM_T(LmtDistOpResponse, ("%s: Seems like the request wasn't even sent ... (%s)", distOpP->regP->regId, distOpP->regP->ipAndPort));

    int         statusCode = 500;
    const char* title      = "Unable to send distributed request";
    const char* detail     = curlToBrokerStrerror(msgP->easy_handle, msgP->data.result, &statusCode);

    LM_E(("CURL Error %d: %s (%s)", msgP->data.result, curl_easy_strerror(msgP->data.result), detail));

    // Mark all attributes of this DistOp as erroneous (those attrs that were sent)
    distOpFailure(responseBody, distOpP, title, detail, statusCode, NULL);
  }
  else if (httpResponseCode == 200)
    LM_W(("%s: unexpected status code %d (using the accumulator?)", distOpP->regP->regId, httpResponseCode));
  else
    LM_W(("%s: unexpected status code %d", distOpP->regP->regId, httpResponseCode));
}



// -----------------------------------------------------------------------------
//
// distOpResponseAccumulate -
//
void distOpResponseAccumulate(DistOp* distOpP, KjNode* responseBody, KjNode* successV, KjNode* failureV, CURLMsg* msgP)
{
  uint64_t  httpResponseCode = 500;

  curl_easy_getinfo(msgP->easy_handle, CURLINFO_RESPONSE_CODE, &httpResponseCode);

  LM_T(LmtDistOpResponse, ("Reg %s: Operation Type:            %s",   distOpP->regP->regId, distOpTypes[distOpP->operation]));
  LM_T(LmtDistOpResponse, ("Reg %s: Distributed Response Code: %d",   distOpP->regP->regId, httpResponseCode));
  LM_T(LmtDistOpResponse, ("Reg %s: Distributed Response Body: '%s'", distOpP->regP->regId, distOpP->rawResponse));

  if ((distOpP->operation == DoCreateEntity) ||
      (distOpP->operation == DoUpdateEntity) ||
      (distOpP->operation == DoDeleteAttrs)  ||
      (distOpP->operation == DoDeleteEntity) ||
      (distOpP->operation == DoMergeEntity)  ||
      (distOpP->operation == DoUpdateAttrs)  ||
      (distOpP->operation == DoReplaceAttr)  ||
      (distOpP->operation == DoAppendAttrs))
  {
    entityResponseAccumulate(distOpP, responseBody, successV, failureV, httpResponseCode, msgP);
  }
  else
  {
    //
    // Not Implemented ...
    //
    LM_W(("operation: %s (real implementation pending - running in \"fall-back\")", distOpTypes[distOpP->operation]));
    if (msgP->data.result == CURLE_OK)
    {
      if ((httpResponseCode >= 200) && (httpResponseCode <= 299))
        distOpSuccess(responseBody, distOpP, NULL, NULL);
      else if (httpResponseCode == 404)
        distOpFailure(responseBody, distOpP, "Not Found", NULL, 404, NULL);
    }
    else
    {
      int         statusCode = 500;
      const char* title      = "Unable to send distributed request";
      const char* detail     = curlToBrokerStrerror(msgP->easy_handle, msgP->data.result, &statusCode);

      LM_E(("CURL Error %d: %s (%s)", msgP->data.result, curl_easy_strerror(msgP->data.result), detail));

      // Mark all attributes of this DistOp as erroneous (those attrs that were sent)
      distOpFailure(responseBody, distOpP, title, detail, statusCode, NULL);
    }
  }
}



// -----------------------------------------------------------------------------
//
// distOpResponses -
//
void distOpResponses(DistOp* distOpList, KjNode* responseBody)
{
  CURLMsg* msgP;
  int      msgsLeft;
  KjNode*  successV = kjLookup(responseBody, "success");
  KjNode*  failureV = kjLookup(responseBody, "failure");

  if (successV == NULL)
  {
    successV = kjArray(orionldState.kjsonP, "success");
    kjChildAdd(responseBody, successV);
  }

  if (failureV == NULL)
  {
    failureV = kjArray(orionldState.kjsonP, "failure");
    kjChildAdd(responseBody, failureV);
  }

  //
  // First, gather all errors from not-sent requests
  //
  int sent = 0;
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    // Perhaps we need a special xError - for the distop requests that weren't even attempted (409 due to no-op of Exclusive reg)
    if (distOpP->error == true)
      distOpFailure(responseBody, distOpP, distOpP->title, distOpP->detail, distOpP->httpResponseCode, NULL);
    else
      ++sent;
  }

  if (sent == 0)
    return;

  //
  // Read responses
  //
  while ((msgP = curl_multi_info_read(orionldState.curlDoMultiP, &msgsLeft)) != NULL)
  {
    if (msgP->msg != CURLMSG_DONE)
      continue;

    DistOp* distOpP = distOpLookupByCurlHandle(distOpList, msgP->easy_handle);

    if (distOpP != NULL)
    {
      LM_T(LmtDistOpResponseDetail, ("%s: got some response - accumulating it", distOpP->regP->regId));
      distOpResponseAccumulate(distOpP, responseBody, successV, failureV, msgP);
    }
    else
      LM_W(("distOpLookupByCurlHandle failed to find the DistOp - the response from the distributed request will not be handled!!!"));
  }
}
