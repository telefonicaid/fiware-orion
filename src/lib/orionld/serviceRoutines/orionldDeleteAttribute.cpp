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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjObject
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/responseFix.h"                          // responseFix
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/legacyDriver/legacyDeleteAttribute.h"          // legacyDeleteAttribute
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/mongoc/mongocEntityGet.h"                      // mongocEntityGet
#include "orionld/mongoc/mongocAttributeDelete.h"                // mongocAttributeDelete
#include "orionld/regMatch/regMatchForEntityGet.h"               // regMatchForEntityGet
#include "orionld/distOp/distOpListsMerge.h"                     // distOpListsMerge
#include "orionld/distOp/distOpSend.h"                           // distOpSend
#include "orionld/distOp/distOpResponses.h"                      // distOpResponses
#include "orionld/distOp/distOpFailure.h"                        // distOpFailure
#include "orionld/distOp/distOpSuccess.h"                        // distOpSuccess
#include "orionld/distOp/distOpLookupByCurlHandle.h"             // distOpLookupByCurlHandle
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease
#include "orionld/distOp/xForwardedForCompose.h"                 // xForwardedForCompose
#include "orionld/distOp/viaCompose.h"                           // viaCompose
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"      // Own interface



// -----------------------------------------------------------------------------
//
// distributedDelete -
//
static DistOp* distributedDelete(KjNode* responseBody, char* entityId, char* entityTypeExpanded, char* entityTypeCompacted, char* attrNameExpanded)
{
  //
  // regMatchForEntityGet needs a StrringArray of the attribute names, as 5th parameter.
  // For DELETE Attribute, there's just a single attribute. but, the array is needed, so ...
  //
  StringArray attrV;
  char*       array[1];

  attrV.items = 1;
  attrV.array = array;
  array[0]    = attrNameExpanded;

  DistOp* exclusiveList = regMatchForEntityGet(RegModeExclusive, DoDeleteAttrs, entityId, entityTypeExpanded, &attrV, NULL);
  DistOp* redirectList  = regMatchForEntityGet(RegModeRedirect,  DoDeleteAttrs, entityId, entityTypeExpanded, &attrV, NULL);
  DistOp* inclusiveList = regMatchForEntityGet(RegModeInclusive, DoDeleteAttrs, entityId, entityTypeExpanded, &attrV, NULL);
  DistOp* distOpList;

  distOpList = distOpListsMerge(exclusiveList,  redirectList);
  distOpList = distOpListsMerge(distOpList, inclusiveList);

  if (distOpList == NULL)
    return NULL;

  // Enqueue all forwarded requests
  // Now that we've found all matching registrations we can add ourselves to the X-forwarded-For header
  char* xff = xForwardedForCompose(orionldState.in.xForwardedFor, localIpAndPort);
  char* via = viaCompose(orionldState.in.via, brokerId);

  int forwards = 0;
  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    // Send the forwarded request and await all responses
    if (distOpP->regP != NULL)
    {
      char dateHeader[70];
      snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);

      if (distOpSend(distOpP, dateHeader, xff, via, false, NULL) == 0)
      {
        ++forwards;
        distOpP->error = false;
      }
      else
      {
        LM_W(("Reg %s: Forwarded request failed", distOpP->regP->regId));
        distOpP->error = true;
      }
    }
  }


  //
  // FIXME:   Try to break the function in two, right here, and to local mongoc stuff in between
  //

  int stillRunning = 1;
  int loops        = 0;

  while (stillRunning != 0)
  {
    CURLMcode cm = curl_multi_perform(orionldState.curlDoMultiP, &stillRunning);
    if (cm != 0)
    {
      LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
      forwards = 0;
      break;
    }

    if (stillRunning != 0)
    {
      cm = curl_multi_wait(orionldState.curlDoMultiP, NULL, 0, 1000, NULL);
      if (cm != CURLM_OK)
      {
        LM_E(("Internal Error (curl_multi_wait: error %d", cm));
        break;
      }
    }

    if ((++loops >= 50) && ((loops % 25) == 0))
      LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
  }

  if (loops >= 100)
    LM_W(("curl_multi_perform finally finished!   (%d loops)", loops));

  // Wait for responses
  if (forwards > 0)
  {
    // Before we can call distOpResponses, all DistOp's in distOpList musdt have the attribute name
    for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
    {
      distOpP->attrName = orionldState.in.pathAttrExpanded;
    }

    distOpResponses(distOpList, responseBody);
  }

  return distOpList;
}



// ----------------------------------------------------------------------------
//
// orionldDeleteAttribute -
//
bool orionldDeleteAttribute(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyDeleteAttribute();

  char* entityId   = orionldState.wildcard[0];
  char* attrName   = orionldState.wildcard[1];

  //
  // Make sure the Entity ID is a valid URI
  //
  if (pCheckUri(entityId, "Entity ID from URL PATH", true) == false)
    return false;

  //
  // Make sure the Attribute Name is valid
  //
  if (pCheckUri(attrName, "Attribute Name from URL PATH", false) == false)
    return false;


  //
  // mhdConnectionTreat() expands the attribute name for us.
  // Here we save it in the orionldState.wildcard array, so that TRoE won't have to expand it
  //
  orionldState.wildcard[1] = orionldState.in.pathAttrExpanded;

  //
  // Retrieve part of the entity from the database (only attrNames)
  //
  const char* projection[]       = { "attrNames", "attrs", NULL };
  KjNode*     dbEntityP          = mongocEntityGet(entityId, projection, false);
  KjNode*     attrNamesP         = NULL;
  KjNode*     attrNameP          = NULL;
  char*       entityTypeExpanded = NULL;
  KjNode*     responseBody       = kjObject(orionldState.kjsonP, NULL);

  if (dbEntityP == NULL)
  {
    if (orionldState.distributed == false)
    {
      orionldError(OrionldResourceNotFound, "Entity Not Found", entityId, 404);
      return false;
    }
    else
      distOpFailure(responseBody, NULL, "Attribute Not Found", NULL, 404, attrName);
  }

  if (dbEntityP != NULL)
  {
    attrNamesP = kjLookup(dbEntityP, "attrNames");

    if (attrNamesP == NULL)
    {
      orionldError(OrionldInternalError, "Database Error (attrNames field not present in database)", entityId, 500);
      return false;
    }

    attrNameP = (attrNamesP != NULL)? kjStringValueLookupInArray(attrNamesP, orionldState.in.pathAttrExpanded) : NULL;
    if (attrNameP == NULL)
    {
      if (orionldState.distributed == false)
      {
        orionldError(OrionldResourceNotFound, "Entity/Attribute not found", orionldState.in.pathAttrExpanded, 404);
        return false;
      }
      else
        distOpFailure(responseBody, NULL, "Attribute Not Found", NULL, 404, attrName);
    }

    KjNode* _idP       = kjLookup(dbEntityP, "_id");
    KjNode* typeP      = (_idP != NULL)? kjLookup(_idP, "type") : NULL;

    entityTypeExpanded  = (typeP != NULL)? typeP->value.s : NULL;  // Always Expanded in DB
  }

  DistOp* distOpList = NULL;
  if (orionldState.distributed == true)
  {
    char* entityTypeCompacted = NULL;
    if (entityTypeExpanded != NULL)
      entityTypeCompacted = orionldContextItemAliasLookup(orionldState.contextP, entityTypeExpanded, NULL, NULL);

    distOpList = distributedDelete(responseBody, entityId, entityTypeExpanded, entityTypeCompacted, orionldState.in.pathAttrExpanded);
  }

  if (attrNameP != NULL)
  {
    int r = mongocAttributeDelete(entityId, orionldState.in.pathAttrExpanded);
    if (r == false)
    {
      if (distOpList == NULL)
      {
        orionldError(OrionldInternalError, "Database Error (deleting attribute from entity)", entityId, 500);
        return false;
      }
      else
        distOpFailure(responseBody, NULL, "Database Error", "(ToDo: get error from mongoc)", 500, attrName);
    }
    else
      distOpSuccess(responseBody, NULL, entityId, attrName);
  }

  if (distOpList != NULL)
    distOpListRelease(distOpList);

  responseFix(responseBody, DoDeleteAttrs, 204, entityId);

  return true;
}
