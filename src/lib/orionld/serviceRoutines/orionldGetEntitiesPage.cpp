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
#include "kalloc/kaAlloc.h"                                         // kaAlloc
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjRender.h"                                         // kjFastRender (for debugging purposes - LM_T)
#include "kjson/kjBuilder.h"                                        // kjArray, ...
#include "kjson/kjLookup.h"                                         // kjLookup
}

#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/types/DistOp.h"                                   // DistOp
#include "orionld/common/orionldState.h"                            // orionldState, entityMaps
#include "orionld/common/orionldError.h"                            // orionldError
#include "orionld/kjTree/kjChildCount.h"                            // kjChildCount
#include "orionld/distOp/distOpLookupByRegId.h"                     // distOpLookupByRegId
#include "orionld/distOp/distOpListDebug.h"                         // distOpListDebug
#include "orionld/distOp/distOpsSend.h"                             // distOpsSend2
#include "orionld/distOp/distOpItemListDebug.h"                     // distOpItemListDebug
#include "orionld/distOp/distOpListItemAdd.h"                       // distOpListItemAdd
#include "orionld/distOp/distOpResponseMergeIntoEntityArray.h"      // distOpResponseMergeIntoEntityArray
#include "orionld/distOp/distOpsSendAndReceive.h"                   // distOpsSendAndReceive
#include "orionld/serviceRoutines/orionldGetEntitiesLocal.h"        // orionldGetEntitiesLocal
#include "orionld/serviceRoutines/orionldGetEntitiesPage.h"         // Own interface



// -----------------------------------------------------------------------------
//
// cleanupSysAttrs -
//
static void cleanupSysAttrs(void)
{
  LM_T(LmtSR, ("In cleanupSysAttrs: orionldState.responseTree: %p", orionldState.responseTree));

  for (KjNode* entityP = orionldState.responseTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* attrP = entityP->value.firstChildP;
    KjNode* nextAttrP;

    while (attrP != NULL)
    {
      nextAttrP = attrP->next;

      LM_T(LmtSR, ("attrP->name: '%s'", attrP->name));
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
// queryResponse -
//
static int queryResponse(DistOp* distOpP, void* callbackParam)
{
  KjNode* entityArray = (KjNode*) callbackParam;

  distOpResponseMergeIntoEntityArray(distOpP, entityArray);
  return 0;
}



// ----------------------------------------------------------------------------
//
// orionldGetEntitiesPage -
//
bool orionldGetEntitiesPage(void)
{
  uint32_t  offset      = orionldState.uriParams.offset;
  uint32_t  limit       = orionldState.uriParams.limit;
  KjNode*   entityArray = kjArray(orionldState.kjsonP, NULL);

  LM_T(LmtEntityMap, ("entity map:          '%s'", orionldState.in.entityMap->id));
  LM_T(LmtEntityMap, ("items in entity map:  %d",  orionldState.in.entityMap->count));
  LM_T(LmtEntityMap, ("offset:               %d",  offset));
  LM_T(LmtEntityMap, ("limit:                %d",  limit));

  // HTTP Status code and payload body
  orionldState.responseTree   = entityArray;
  LM_T(LmtSR, ("orionldState.responseTree: %p", orionldState.responseTree));
  orionldState.httpStatusCode = 200;

  if (orionldState.uriParams.count == true)
  {
    LM_T(LmtEntityMap, ("%d entities match, in the entire federation", orionldState.in.entityMap->count));
    LM_T(LmtEntityMap, ("COUNT: Adding HttpResultsCount header: %d", orionldState.in.entityMap->count));
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, orionldState.in.entityMap->count);
  }

  if (offset >= orionldState.in.entityMap->count)
  {
    LM_T(LmtEntityMap, ("offset (%d) >= orionldState.in.entityMap->count (%d)", offset, orionldState.in.entityMap->count));
    return true;
  }

  //
  // Fast forward to offset index in the KJNode array that is the entity map
  //
  KjNode* entityMap = orionldState.in.entityMap->map->value.firstChildP;

  for (uint32_t ix = 0; ix < offset; ix++)
  {
    entityMap = entityMap->next;
  }

  //
  // entityMap now points to the first entity to give back.
  // Must extract all parts of the entities, according to their array inside the Entity Map,
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
  //     "@none":    [ "urn:E1", ... "urn:En" ]
  //   }
  // ]
  //
  // Once that array is ready, we can send forwarded requests or query the local DB
  //
  KjNode* sources = kjObject(orionldState.kjsonP, NULL);

  for (uint32_t ix = 0; ix < limit; ix++)
  {
    if (entityMap == NULL)  // in case we have less than "limit"
      break;

    char* entityId = entityMap->name;

    for (KjNode* regP = entityMap->value.firstChildP; regP != NULL; regP = regP->next)
    {
      const char* regId    = (regP->type == KjString)? regP->value.s : "@none";
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

  DistOpListItem* distOpListItem = NULL;
  for (KjNode* sourceP = sources->value.firstChildP; sourceP != NULL; sourceP = sourceP->next)
  {
    if (strcmp(sourceP->name, "@none") == 0)
    {
      LM_T(LmtDistOpList, ("orionldState.distOpList at %p", orionldState.distOpList));
      DistOp* distOpP = distOpLookupByRegId(orionldState.distOpList, "@none");

      // Local query - set input params for orionldGetEntitiesLocal

      // We want the entire entity this time, not only the entity id
      orionldState.uriParams.onlyIds = false;

      // The type doesn't matter, we have a list of entity ids
      orionldState.in.typeList.items = 0;

#if 0
      LM_T(LmtDistOpAttributes, ("------------ Local DB Query -------------"));
      LM_T(LmtDistOpAttributes, ("orionldState.uriParams.attrs:   %s", orionldState.uriParams.attrs));
      LM_T(LmtDistOpAttributes, ("orionldState.in.attrList.items: %d", orionldState.in.attrList.items));
      for (int ix = 0; ix < orionldState.in.attrList.items; ix++)
      {
        LM_T(LmtDistOpAttributes, ("orionldState.in.attrList.array[%d]: '%s'", ix, orionldState.in.attrList.array[ix]));
      }
      LM_T(LmtDistOpAttributes, ("distOpP->attrsParam:          %s", distOpP->attrsParam));
      LM_T(LmtDistOpAttributes, ("distOpP->attrList:            %p", distOpP->attrList));

      if (distOpP->attrList != NULL)
      {
        LM_T(LmtDistOpAttributes, ("distOpP->attrList->items: %d", distOpP->attrList->items));
        for (int ix = 0; ix < distOpP->attrList->items; ix++)
        {
          LM_T(LmtDistOpAttributes, ("distOpP->attrList->array[%d]: '%s'", ix, distOpP->attrList->array[ix]));
        }
      }
#endif

      // Set orionldState.in.geometryPropertyExpanded according to the entityMap creation request (that's modifying the response)
      orionldState.in.geometryPropertyExpanded = NULL;  // FIXME: fix

      // Set orionldState.in.idList according to the entities in the entityMap
      idListFix(sourceP);

      // Use orionldState.in.attrList and not distOpP->attrList for local requests

      // No paging here
      orionldState.uriParams.offset = 0;
      orionldState.uriParams.limit  = orionldState.in.idList.items;

      LM_T(LmtEntityMap, ("Query local database for %d entities", orionldState.in.idList.items));
      orionldGetEntitiesLocal(distOpP->typeList,
                              distOpP->idList,
                              &orionldState.in.attrList,
                              NULL,
                              distOpP->qNode,
                              &distOpP->geoInfo,
                              distOpP->lang,
                              true,                        // sysAttrs needed, to help pick attributes in case more than one of the same
                              distOpP->geometryProperty,
                              false,
                              true);

      // Response comes in orionldState.responseTree - move those to entityArray
      if ((orionldState.responseTree != NULL) && (orionldState.responseTree->value.firstChildP != NULL))
      {
        LM_T(LmtDistOpResponseDetail, ("Adding a 'distop-response-entity' to the entityArray"));

        orionldState.responseTree->lastChild->next = entityArray->value.firstChildP;
        entityArray->value.firstChildP = orionldState.responseTree->value.firstChildP;
        if (entityArray->lastChild == NULL)
          entityArray->lastChild = orionldState.responseTree->lastChild;
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
      distOpListItem = distOpListItemAdd(distOpListItem, sourceP->name, idString);
    }
  }

  if (distOpListItem != NULL)
  {
    distOpItemListDebug(distOpListItem, "To Forward for GET /entities");
    distOpsSendAndReceive(distOpListItem, queryResponse, entityArray);
  }

  orionldState.responseTree = entityArray;

  //
  // Time to cleanup ...
  //
  // 1. Remove all timestamps, unless requested
  //
  if (orionldState.uriParamOptions.sysAttrs == false)
    cleanupSysAttrs();

  return true;
}
