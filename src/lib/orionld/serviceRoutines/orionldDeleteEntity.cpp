/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
* Author: Ken Zangelin and Gabriel Quaresma
*/
#include <string>
#include <vector>

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/responseFix.h"                          // responseFix
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_URI
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocEntityDelete.h"                   // mongocEntityDelete
#include "orionld/notifications/orionldAlterations.h"            // orionldAlterations
#include "orionld/regMatch/regMatchForEntityGet.h"               // regMatchForEntityGet
#include "orionld/distOp/distOpListsMerge.h"                     // distOpListsMerge
#include "orionld/distOp/distOpSend.h"                           // distOpSend
#include "orionld/distOp/distOpLookupByCurlHandle.h"             // distOpLookupByCurlHandle
#include "orionld/distOp/distOpResponses.h"                      // distOpResponses
#include "orionld/distOp/distOpSuccess.h"                        // distOpSuccess
#include "orionld/distOp/distOpFailure.h"                        // distOpFailure
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease
#include "orionld/distOp/xForwardedForCompose.h"                 // xForwardedForCompose
#include "orionld/distOp/viaCompose.h"                           // viaCompose
#include "orionld/serviceRoutines/orionldDeleteEntity.h"         // Own Interface



// -----------------------------------------------------------------------------
//
// alterations -
//
static void alterations(char* entityId, char* entityTypeExpanded, char* entityTypeCompacted)
{
  //
  // Create the alteration for notifications
  //
  orionldState.alterations = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));
  bzero(orionldState.alterations, sizeof(OrionldAlteration));
  orionldState.alterations->entityId   = entityId;
  orionldState.alterations->entityType = entityTypeExpanded;

  //
  // Create a payload body to represent the deletion (to be sent as notification later)
  //
  KjNode* apiEntityP      = kjObject(orionldState.kjsonP, NULL);  // Used for notifications, if needed
  KjNode* idNodeP         = kjString(orionldState.kjsonP, "id", entityId);
  KjNode* deletedAtNodeP  = kjString(orionldState.kjsonP, "deletedAt", orionldState.requestTimeString);

  kjChildAdd(apiEntityP, idNodeP);
  if (entityTypeCompacted != NULL)
  {
    KjNode* typeNodeP    = kjString(orionldState.kjsonP, "type", entityTypeCompacted);
    kjChildAdd(apiEntityP, typeNodeP);
  }
  kjChildAdd(apiEntityP, deletedAtNodeP);

  orionldState.alterations->finalApiEntityP = apiEntityP;
}



// -----------------------------------------------------------------------------
//
// distributedDelete -
//
static DistOp* distributedDelete(char* entityId, char* entityTypeExpanded, char* entityTypeCompacted)
{
  DistOp* exclusiveList = regMatchForEntityGet(RegModeExclusive, DoDeleteEntity, entityId, entityTypeExpanded, NULL, NULL);
  DistOp* redirectList  = regMatchForEntityGet(RegModeRedirect,  DoDeleteEntity, entityId, entityTypeExpanded, NULL, NULL);
  DistOp* inclusiveList = regMatchForEntityGet(RegModeInclusive, DoDeleteEntity, entityId, entityTypeExpanded, NULL, NULL);
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

  return distOpList;
}



// ----------------------------------------------------------------------------
//
// orionldDeleteEntity -
//
bool orionldDeleteEntity(void)
{
  char*   entityId            = orionldState.wildcard[0];
  char*   entityTypeExpanded  = orionldState.uriParams.type;
  char*   entityTypeCompacted = NULL;
  KjNode* responseBody        = kjObject(orionldState.kjsonP, NULL);

  // Make sure the Entity ID is a valid URI
  PCHECK_URI(entityId, true, 0, "Invalid Entity ID", "Must be a valid URI", 400);

  if (orionldState.uriParams.type != NULL)
    entityTypeExpanded = orionldContextItemExpand(orionldState.contextP, orionldState.uriParams.type, true, NULL);

  //
  // GET the entity locally
  //   FIXME: Overkill to extract the entire entity from the DB
  //
  KjNode* dbEntityP  = mongocEntityLookup(entityId, entityTypeExpanded, NULL, NULL, NULL);

  //
  // More info on Entity Type
  //
  // - If given as URI param, we must expand it according to the @context
  // - If entity type not given as URI param, but the entity is found in the DB - extract it (the entity type) AND compact it
  // - Also, if given as URI param, we might need to compact it back to shortname according to the @context
  //
  if ((orionldState.uriParams.type == NULL) && (dbEntityP != NULL))
  {
    KjNode* _idP  = kjLookup(dbEntityP, "_id");
    KjNode* typeP = (_idP != NULL)? kjLookup(_idP, "type") : NULL;

    entityTypeExpanded  = (typeP != NULL)? typeP->value.s : NULL;  // Always Expanded in DB
  }

  if (entityTypeExpanded != NULL)
    entityTypeCompacted = orionldContextItemAliasLookup(orionldState.contextP, entityTypeExpanded, NULL, NULL);

  DistOp* distOpList = (orionldState.distributed == false)? NULL : distributedDelete(entityId, entityTypeExpanded, entityTypeCompacted);

  //
  // Delete the entity in the local DB
  // - Error if the entity is not found locally, and not subject to forwarding
  //
  if (dbEntityP == NULL)
  {
    if (distOpList == NULL)
    {
      orionldError(OrionldResourceNotFound, "Entity not found", entityId, 404);
      return false;
    }
    else
      distOpFailure(responseBody, NULL, "Not Found", entityId, 404, NULL);
  }

  //
  // Delete the entity locally (if present)
  // Give 404 if the entity is not present locally nor triggered any forwarded requests
  //
  char* detail = NULL;
  if (dbEntityP != NULL)
  {
    if (mongocEntityDelete(entityId, &detail) == true)
    {
      // Add a success to the "success" member
      distOpSuccess(responseBody, NULL, entityId, NULL);
    }
    else
    {
      if (distOpList == NULL)  // pure local request
      {
        orionldError(OrionldInternalError, "Database Error", detail, 500);
        return false;
      }
      else
        distOpFailure(responseBody, NULL, "Database Error", detail, 500, NULL);
    }
  }

  if (dbEntityP != NULL)
    alterations(entityId, entityTypeExpanded, entityTypeCompacted);

  if (distOpList != NULL)
  {
    distOpResponses(distOpList, responseBody);
    distOpListRelease(distOpList);
  }

  kjTreeLog(responseBody, "responseBody", LmtSR);
  responseFix(responseBody, DoDeleteEntity, 204, entityId);

  return true;
}
