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
extern "C"
{
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjRender.h"                                         // kjFastRender (for debugging purposes - LM_T)
#include "kjson/kjBuilder.h"                                        // kjArray, ...
#include "kjson/kjLookup.h"                                         // kjLookup
#include "kjson/kjParse.h"                                          // kjParse
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // LmtMongoc

#include "orionld/common/orionldState.h"                            // orionldState, orionldEntityMapCount
#include "orionld/common/orionldError.h"                            // orionldError
#include "orionld/kjTree/kjChildCount.h"                            // kjChildCount
#include "orionld/regCache/regCacheItemLookup.h"                    // regCacheItemLookup
#include "orionld/forwarding/DistOp.h"                              // DistOp
#include "orionld/forwarding/distOpCreate.h"                        // distOpCreate
#include "orionld/forwarding/distOpListDebug.h"                     // distOpListDebug
#include "orionld/forwarding/distOpEntityMerge.h"                   // distOpEntityMerge
#include "orionld/forwarding/distOpsSend.h"                         // distOpsSend2
#include "orionld/forwarding/distOpLookupByCurlHandle.h"            // distOpLookupByCurlHandle
#include "orionld/serviceRoutines/orionldGetEntitiesLocal.h"        // orionldGetEntitiesLocal
#include "orionld/serviceRoutines/orionldGetEntitiesPage.h"         // Own interface



// -----------------------------------------------------------------------------
//
// distOpItemListDebug - move!
//
void distOpItemListDebug(DistOpListItem* distOpList, const char* msg)
{
  LM_T(LmtDistOpList, ("------------- %s -----------------", msg));
  DistOpListItem* itemP = distOpList;
  while (itemP != NULL)
  {
    LM_T(LmtDistOpList, ("  DistOp:   %s", itemP->distOpP->id));
    LM_T(LmtDistOpList, ("  Entities: %s", itemP->entityIds));
    LM_T(LmtDistOpList, ("  ------------------------------"));

    itemP = itemP->next;
  }
}



// -----------------------------------------------------------------------------
//
// idListFix -
//
static void idListFix(KjNode* entityIdArray)
{
  int entityIds = kjChildCount(entityIdArray);

  orionldState.in.idList.items = entityIds;
  orionldState.in.idList.array = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * entityIds);

  KjNode* entityIdP = entityIdArray->value.firstChildP;
  for (int ix = 0; ix < entityIds; ix++)
  {
    orionldState.in.idList.array[ix] = entityIdP->value.s;
    entityIdP = entityIdP->next;
  }
}



// -----------------------------------------------------------------------------
//
// distOpLookup -
//
DistOp* distOpLookup(const char* distOpId)
{
  DistOp* distOpP = orionldDistOps;

  while (distOpP != NULL)
  {
    if (strcmp(distOpP->id, distOpId) == 0)  // This could be greatly improved.  E.g. distOpId to be the index in the array ...
      return distOpP;

    distOpP = distOpP->next;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// distOpListItemCreate -
//
DistOpListItem* distOpListItemCreate(const char* distOpId, char* idString)
{
  DistOpListItem* itemP = (DistOpListItem*) kaAlloc(&orionldState.kalloc, sizeof(DistOpListItem));

  if (itemP == NULL)
    LM_X(1, ("Out of memory"));

  itemP->distOpP = distOpLookup(distOpId);

  if (itemP->distOpP == NULL)
    LM_RE(NULL, ("Internal Error (unable to find the DistOp '%s'", distOpId));

  itemP->next      = NULL;
  itemP->entityIds = idString;

  return itemP;
}



// -----------------------------------------------------------------------------
//
// distOpListItemAdd -
//
DistOpListItem* distOpListItemAdd(DistOpListItem* distOpList, const char* distOpId, char* idString)
{
  LM_T(LmtEntityMap, ("Creating DistOpListItem for DistOp '%s', entities '%s'", distOpId, idString));

  DistOpListItem* doliP = distOpListItemCreate(distOpId, idString);

  if (doliP == NULL)
    return distOpList;

  if (distOpList != NULL)
    doliP->next = distOpList;

  return doliP;
}



// -----------------------------------------------------------------------------
//
// queryResponse -
//
static int queryResponse(DistOp* distOpP, void* callbackParam)
{
  LM_T(LmtSR, ("Got a response. status code: %d. body: '%s'", distOpP->httpResponseCode, distOpP->rawResponse));
  KjNode* entityArray = (KjNode*) callbackParam;
  kjTreeLog(distOpP->responseBody, "Response", LmtSR);

  if ((distOpP->httpResponseCode == 200) && (distOpP->responseBody != NULL))
  {
    LM_T(LmtSR, ("Got a body from endpoint registered in reg '%s'", distOpP->regP->regId));
    LM_T(LmtSR, ("Must merge these new entities with the once already received"));

    KjNode* entityP = distOpP->responseBody->value.firstChildP;
    KjNode* next;
    while (entityP != NULL)
    {
      next = entityP->next;
      kjChildRemove(distOpP->responseBody, entityP);
      distOpEntityMerge(entityArray, entityP);
      entityP = next;
    }
  }

  return 0;
}



//
// FIXME: these two functions need to get their own modules in orionld/forwarding
//        Move from orionldGetEntitiesDistributed.cpp
//
typedef int (*DistOpResponseTreatFunction)(DistOp* distOpP, void* callbackParam);



// -----------------------------------------------------------------------------
//
// distOpsReceive2 - FIXME: move to orionld/forwarding/distOpsReceive.cpp/h
//
void distOpsReceive2(DistOpResponseTreatFunction treatFunction, void* callbackParam)
{
  LM_T(LmtSR, ("Receiving responses"));
  //
  // Read the responses to the forwarded requests
  //
  CURLMsg* msgP;
  int      msgsLeft;

  while ((msgP = curl_multi_info_read(orionldState.curlDoMultiP, &msgsLeft)) != NULL)
  {
    if (msgP->msg != CURLMSG_DONE)
      continue;

    if (msgP->data.result == CURLE_OK)
    {
      DistOp* distOpP = distOpLookupByCurlHandle(orionldDistOps, msgP->easy_handle);

      if (distOpP == NULL)
      {
        LM_E(("Unable to find the curl handle of a message, presumably a response to a forwarded request"));
        continue;
      }

      curl_easy_getinfo(msgP->easy_handle, CURLINFO_RESPONSE_CODE, &distOpP->httpResponseCode);

      if ((distOpP->rawResponse != NULL) && (distOpP->rawResponse[0] != 0))
        distOpP->responseBody = kjParse(orionldState.kjsonP, distOpP->rawResponse);

      LM_T(LmtDistOpResponse, ("%s: received a response for a forwarded request", distOpP->regP->regId, distOpP->httpResponseCode));
      LM_T(LmtDistOpResponse, ("%s: response for a forwarded request: %s", distOpP->regP->regId, distOpP->rawResponse));

      treatFunction(distOpP, callbackParam);
    }
  }
}



// -----------------------------------------------------------------------------
//
// distOpQueryRequest -
//
static void distOpQueryRequest(DistOpListItem* distOpList, KjNode* entityArray)
{
  // Send all distributed requests
  LM_T(LmtSR, ("Calling distOpsSend2"));
  
  int forwards = distOpsSend2(distOpList);
  LM_T(LmtSR, ("distOpsSend2 says %d forwards", forwards));

  // Await all responses, if any
  if (forwards > 0)
    distOpsReceive2(queryResponse, entityArray);
}



// -----------------------------------------------------------------------------
//
// cleanupSysAttrs - 
//
static void cleanupSysAttrs(void)
{
  for (KjNode* entityP = orionldState.responseTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* attrP = entityP->value.firstChildP;
    KjNode* nextAttrP;

    while (attrP != NULL)
    {
      nextAttrP = attrP->next;

      if      (strcmp(attrP->name, "createdAt")  == 0)  kjChildRemove(entityP, attrP);
      else if (strcmp(attrP->name, "modifiedAt") == 0)  kjChildRemove(entityP, attrP);
      else if (attrP->type == KjObject)
      {
        // It's an attribute        

        KjNode* subAttrP = attrP->value.firstChildP;
        KjNode* nextSubAttrP;

        while (subAttrP != NULL)
        {
          nextSubAttrP = subAttrP->next;

          if      (strcmp(subAttrP->name, "createdAt")  == 0)  kjChildRemove(attrP, subAttrP);
          else if (strcmp(subAttrP->name, "modifiedAt") == 0)  kjChildRemove(attrP, subAttrP);

          subAttrP = nextSubAttrP;
        }

      }

      attrP = nextAttrP;
    }
  }
}



// ----------------------------------------------------------------------------
//
// orionldGetEntitiesPage -
//
bool orionldGetEntitiesPage(void)
{
  int offset = orionldState.uriParams.offset;
  int limit  = orionldState.uriParams.limit;

  KjNode* entityArray = kjArray(orionldState.kjsonP, NULL);

  LM_T(LmtEntityMap, ("entity map:          '%s'", orionldEntityMapId));
  LM_T(LmtEntityMap, ("items in entity map:  %d",  orionldEntityMapCount));
  LM_T(LmtEntityMap, ("offset:               %d",  offset));
  LM_T(LmtEntityMap, ("limit:                %d",  limit));

  // HTTP Status code and payload body
  orionldState.responseTree   = entityArray;
  orionldState.httpStatusCode = 200;

  if (orionldState.uriParams.count == true)
  {
    LM_T(LmtEntityMap, ("%d entities match, in the entire federation", orionldEntityMapCount));
    LM_T(LmtEntityMap, ("COUNT: Adding HttpResultsCount header: %d", orionldEntityMapCount));
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, orionldEntityMapCount);
  }

  if (offset >= orionldEntityMapCount)
  {
    LM_T(LmtEntityMap, ("offset (%d) >= orionldEntityMapCount (%d)", offset, orionldEntityMapCount));
    return true;
  }

  //
  // Fast forward to offset index in the KJNode array that is the entity map
  //
  KjNode* entityMap = orionldEntityMap->value.firstChildP;
  for (int ix = 0; ix < offset; ix++)
  {
    entityMap = entityMap->next;
  }

  //
  // entityMap now points to the first entity to give back.
  // Must extract all parts of the entities, according to their array inside orionldEntityMap,
  // and merge them together (in case of distributed entities
  //

  //
  // What we have here is a number of "slots" in the entity map, each slot with the layout:
  //
  // "urn:cp3:entities:E30" [ "urn:Reg1", "urn:Reg2", null ]
  //
  // To avoid making a DB query, of forwarded request, for each and every entity in the slots,
  // we must here group them into:
  //
  //   {
  //     "urn:reg1": [ "urn:E1", ... "urn:En" ],
  //     "urn:reg2": [ "urn:E1", ... "urn:En" ],
  //     "local":  [ "urn:E1", ... "urn:En" ]
  //   }
  // ]
  //
  // Once that array is ready, we can send forwarded requests or query the local DB
  //
  KjNode* sources = kjObject(orionldState.kjsonP, NULL);

  for (int ix = 0; ix < limit; ix++)
  {
    if (entityMap == NULL)  // in case we have less than "limit"
      break;

    char* entityId = entityMap->name;

    for (KjNode* regP = entityMap->value.firstChildP; regP != NULL; regP = regP->next)
    {
      const char* regId    = (regP->type == KjString)? regP->value.s : "local";
      KjNode*     regArray = kjLookup(sources, regId);

      if (regArray == NULL)
      {
        regArray = kjArray(orionldState.kjsonP, regId);
        kjChildAdd(sources, regArray);
      }

      KjNode* entityIdNodeP = kjString(orionldState.kjsonP, NULL, entityId);
      kjChildAdd(regArray, entityIdNodeP);
    }

    entityMap = entityMap->next;
  }

  kjTreeLog(sources, "sources", LmtEntityMapDetail);

  DistOpListItem* distOpList = NULL;
  for (KjNode* sourceP = sources->value.firstChildP; sourceP != NULL; sourceP = sourceP->next)
  {
    if (strcmp(sourceP->name, "local") == 0)
    {
      DistOp* distOpP = distOpLookup("local");

      // Local query - set input params for orionldGetEntitiesLocal

      // We want the entire entity this time, not only the entity id
      orionldState.uriParams.onlyIds = false;

      // The type doesn't matter, we have a list of entity ids
      orionldState.in.typeList.items = 0;

      // Set orionldState.in.attrList according to the entityMap creation request (that's filtering away attributes)
      orionldState.uriParams.attrs = distOpP->attrsParam;

      // Set orionldState.in.geometryPropertyExpanded according to the entityMap creation request (that's modifying the response)
      orionldState.in.geometryPropertyExpanded = NULL;  // FIXME: fix

      // Set orionldState.in.idList according to the entities in the entityMap
      idListFix(sourceP);

      // No paging here
      orionldState.uriParams.offset = 0;
      orionldState.uriParams.limit  = orionldState.in.idList.items;

      LM_T(LmtEntityMap, ("Query local database for %d entities", orionldState.in.idList.items));
      orionldGetEntitiesLocal(distOpP->typeList,
                              distOpP->idList,
                              distOpP->attrList,
                              NULL,
                              distOpP->qNode,
                              &distOpP->geoInfo,
                              distOpP->lang,
                              true,                        // sysAttrs needed, to help pick attributes in case more than one of the same
                              distOpP->geometryProperty,
                              false,
                              true);

      // Response comes in orionldState.responseTree - move those to entityArray
      kjTreeLog(orionldState.responseTree, "orionldState.responseTree", LmtEntityMapDetail);
      if ((orionldState.responseTree != NULL) && (orionldState.responseTree->value.firstChildP != NULL))
      {
        orionldState.responseTree->lastChild->next = entityArray->value.firstChildP;
        entityArray->value.firstChildP = orionldState.responseTree->value.firstChildP;
      }
    }
    else
    {
      int idStringSize = 0;

      LM_T(LmtEntityMap, ("Query '%s' for:", sourceP->name));
      for (KjNode* entityNodeP = sourceP->value.firstChildP; entityNodeP != NULL; entityNodeP = entityNodeP->next)
      {
        LM_T(LmtEntityMap, ("  o %s", entityNodeP->value.s));
        idStringSize += strlen(entityNodeP->value.s) + 1;  // +1 for the comma
      }

      char* idString   = kaAlloc(&orionldState.kalloc, idStringSize);
      int   idStringIx = 0;

      bzero(idString, idStringSize);
      for (KjNode* entityNodeP = sourceP->value.firstChildP; entityNodeP != NULL; entityNodeP = entityNodeP->next)
      {
        strcpy(&idString[idStringIx], entityNodeP->value.s);
        idStringIx += strlen(entityNodeP->value.s);

        if (entityNodeP->next != NULL)  // No comma if last item
        {
          idString[idStringIx] = ',';
          idStringIx += 1;
        }
      }

      LM_T(LmtEntityMap, ("Query '%s' with entity ids=%s", sourceP->name, idString));
      distOpList = distOpListItemAdd(distOpList, sourceP->name, idString);
    }
  }

  if (distOpList != NULL)
  {
    distOpItemListDebug(distOpList, "To Forward for GET /entities");
    distOpQueryRequest(distOpList, entityArray);
  }


  //
  // Time to cleanup ...
  //
  // 1. Remove all timestamps, unless requested
  //
  if (orionldState.uriParamOptions.sysAttrs == false)
    cleanupSysAttrs();

  return true;
}
