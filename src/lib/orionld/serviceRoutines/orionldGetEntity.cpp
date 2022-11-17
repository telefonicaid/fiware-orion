/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd, kjArray, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/curlToBrokerStrerror.h"                 // curlToBrokerStrerror
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/legacyDriver/legacyGetEntity.h"                // legacyGetEntity
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/kjTree/kjGeojsonEntityTransform.h"             // kjGeojsonEntityTransform
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/forwarding/regMatchForEntityGet.h"             // regMatchForEntityGet
#include "orionld/forwarding/forwardingListsMerge.h"             // forwardingListsMerge
#include "orionld/forwarding/forwardRequestSend.h"               // forwardRequestSend
#include "orionld/forwarding/fwdPendingLookupByCurlHandle.h"     // fwdPendingLookupByCurlHandle
#include "orionld/serviceRoutines/orionldGetEntity.h"            // Own interface



// -----------------------------------------------------------------------------
//
// entityMerge -
//
bool entityMerge(KjNode* apiEntityP, KjNode* body)
{
  KjNode* idP     = kjLookup(body, "id");
  KjNode* typeP   = kjLookup(body, "type");

  if (idP)
    kjChildRemove(body, idP);
  if (typeP)
    kjChildRemove(body, typeP);

  KjNode* attrP   = body->value.firstChildP;
  KjNode* next;

  while (attrP != NULL)
  {
    next = attrP->next;

    KjNode* oldP = kjLookup(apiEntityP, attrP->name);

    if (oldP == NULL)
    {
      kjChildRemove(body, attrP);
      kjChildAdd(apiEntityP, attrP);
    }
    else  // two copies of the same attr ...
    {
      LM_W(("The attribute '%s' is already present ... Need to implement ... something!", attrP->name));
    }

    attrP = next;
  }

  return true;
}



#if 0
static void attrsDebug(StringArray* attrV, const char* info)
{
  LM(("SLIST: %s", info));
  if (attrV != NULL)
  {
    if (attrV->items == 0)
    {
      LM(("SLIST:   EMPTY attrV"));
    }
    else
    {
      for (int ix = 0; ix < attrV->items; ix++)
      {
        LM(("SLIST:   Attr %d: %s", ix, attrV->array[ix]));
      }
    }
  }
  else
    LM(("SLIST:   NULL attrV"));
}

static void attrsDebug2(ForwardPending*  fwdPendingList)
{
  ForwardPending* fwdPendingP = fwdPendingList;

  LM(("SLIST: *************** Entire ForwardPending list ***************************"));
  while (fwdPendingP != NULL)
  {
    KjNode* regIdP = kjLookup(fwdPendingP->regP->regTree, "id");

    LM(("SLIST: -------------------------------------------------------------"));
    LM(("SLIST: Registration: %s", (regIdP != NULL)? regIdP->value.s : "unknown"));
    LM(("SLIST: FwdOperation: %s", fwdOperations[fwdPendingP->operation]));
    attrsDebug(fwdPendingP->attrList, "attrList");
    LM(("SLIST: -------------------------------------------------------------"));

    fwdPendingP = fwdPendingP->next;
  }
  LM(("SLIST: ******************************************************"));
}
#endif


// ----------------------------------------------------------------------------
//
// orionldGetEntity -
//
// URL PARAMETERS
// - attrs
// - geometryProperty
// - lang
// - options=simplified/concise  (normalized is default)
//
bool orionldGetEntity(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyGetEntity();

  const char* entityId = orionldState.wildcard[0];
  if (pCheckUri(entityId, "Entity ID in URL PATH", true) == false)
    return false;

  bool             distributed    = (forwarding == true) || (orionldState.uriParams.local == false);
  ForwardPending*  fwdPendingList = NULL;
  int              forwards       = 0;

  if (distributed)
  {
    StringArray*  attrV   = (orionldState.uriParams.attrs != NULL)? &orionldState.in.attrList : NULL;
    const char*   geoProp = orionldState.uriParams.geometryProperty;

    char dateHeader[70];
    snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);

    //
    // If attrV AND geoProp are set, then geoProp needs to be merged in into attrV
    // UNLESS it's not present already.
    //
    // If attrV is not used BUT geoProp is, then attrV needs to be an array on ONE ITEM: geoProp
    //
    // This must be done as local copies, as otherwiose we fuck it up for the query lin the local database
    //
    //
    // If 'attrs' is used, any matches in Exclusive/Redirect registrations must chop off attributes from 'attrs'
    //
    // We want EVERYTHING from Auxiliary regs (and we may throw it all away in the end),
    // so we create the ForwardPending list FIRST, before any attrs are removed
    //
    ForwardPending* auxiliarList  = regMatchForEntityGet(RegModeAuxiliary, FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* exclusiveList = regMatchForEntityGet(RegModeExclusive, FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* redirectList  = regMatchForEntityGet(RegModeRedirect,  FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* inclusiveList = regMatchForEntityGet(RegModeInclusive, FwdRetrieveEntity, entityId, attrV, geoProp);

    fwdPendingList = forwardingListsMerge(exclusiveList,  redirectList);
    fwdPendingList = forwardingListsMerge(fwdPendingList, inclusiveList);
    fwdPendingList = forwardingListsMerge(fwdPendingList, auxiliarList);

    // Send - copy from orionldPostEntities
    if (fwdPendingList != NULL)
    {
      for (ForwardPending* fwdPendingP = fwdPendingList; fwdPendingP != NULL; fwdPendingP = fwdPendingP->next)
      {
        // Send the forwarded request and await all responses
        if (fwdPendingP->regP != NULL)
        {
          if (forwardRequestSend(fwdPendingP, dateHeader) == 0)
          {
            ++forwards;
            fwdPendingP->error = false;
          }
          else
          {
            LM_W(("Forwarded request failed"));
            fwdPendingP->error = true;
          }
        }
      }

      int stillRunning = 1;
      int loops        = 0;

      while (stillRunning != 0)
      {
        CURLMcode cm = curl_multi_perform(orionldState.curlFwdMultiP, &stillRunning);
        if (cm != 0)
        {
          LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
          forwards = 0;
          break;
        }

        if (stillRunning != 0)
        {
          cm = curl_multi_wait(orionldState.curlFwdMultiP, NULL, 0, 1000, NULL);
          if (cm != CURLM_OK)
          {
            LM_E(("Internal Error (curl_multi_wait: error %d", cm));
            break;
          }
        }

        if ((++loops >= 10) && ((loops % 5) == 0))
          LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
      }
    }
  }

  KjNode* dbEntityP  = mongocEntityLookup(entityId, &orionldState.in.attrList, orionldState.uriParams.geometryProperty);
  KjNode* apiEntityP = NULL;
  if (dbEntityP == NULL)
  {
    if (distributed == false)
    {
      const char* title = (orionldState.in.attrList.items != 0)? "Combination Entity/Attributes Not Found" : "Entity Not Found";
      orionldError(OrionldResourceNotFound, title, entityId, 404);
      return false;
    }
    // If distributed, then it's perfectly OK to have nothing locally
    // BUT, distributes hasn't been implemented yet, so ...
    const char* title = (orionldState.in.attrList.items != 0)? "Combination Entity/Attributes Not Found" : "Entity Not Found";
    orionldError(OrionldResourceNotFound, title, entityId, 404);
    return false;
  }
  else
    apiEntityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, orionldState.out.format, orionldState.uriParams.lang, true, &orionldState.pd);

  //
  // Now read responses to the forwarded requests
  //
  if (forwards > 0)
  {
    CURLMsg* msgP;
    int      msgsLeft;

    while ((msgP = curl_multi_info_read(orionldState.curlFwdMultiP, &msgsLeft)) != NULL)
    {
      if (msgP->msg != CURLMSG_DONE)
        continue;

      if (msgP->data.result == CURLE_OK)
      {
        ForwardPending* fwdPendingP = fwdPendingLookupByCurlHandle(fwdPendingList, msgP->easy_handle);

        LM(("Got a response: %s", fwdPendingP->rawResponse));
        fwdPendingP->body = kjParse(orionldState.kjsonP, fwdPendingP->rawResponse);
        if (fwdPendingP->body != NULL)
        {
          // Merge in the received body into the local (or nothing)
          if (apiEntityP == NULL)
            apiEntityP = fwdPendingP->body;
          else
            entityMerge(apiEntityP, fwdPendingP->body);
        }
        else
          LM_E(("Internal Error (parse error for the received response of a forwarded request)"));
      }
      else
        LM_E(("CURL Error %d awaiting response to forwarded request: %s", msgP->data.result, curl_easy_strerror(msgP->data.result)));
    }
  }

  orionldState.responseTree   = apiEntityP;
  orionldState.httpStatusCode = 200;

  if (orionldState.out.contentType == GEOJSON)
  {
    orionldState.responseTree = kjGeojsonEntityTransform(orionldState.responseTree, orionldState.geoPropertyNode);

    //
    // If URI params 'attrs' and 'geometryProperty' are given BUT 'geometryProperty' is not part of 'attrs', then we need to remove 'geometryProperty' from
    // the "properties" object
    //
    if ((orionldState.in.attrList.items > 0) && (orionldState.uriParams.geometryProperty != NULL))
    {
      bool geometryPropertyInAttrList = false;

      for (int ix = 0; ix < orionldState.in.attrList.items; ix++)
      {
        if (strcmp(orionldState.in.geometryPropertyExpanded, orionldState.in.attrList.array[ix]) == 0)
        {
          geometryPropertyInAttrList = true;
          break;
        }
      }

      if (geometryPropertyInAttrList == false)
      {
        KjNode* propertiesP = kjLookup(orionldState.responseTree, "properties");

        if (propertiesP != NULL)
        {
          KjNode* geometryPropertyP = kjLookup(propertiesP, orionldState.in.geometryPropertyExpanded);

          if (geometryPropertyP != NULL)
            kjChildRemove(propertiesP, geometryPropertyP);
        }
      }
    }
  }

  return true;
}
