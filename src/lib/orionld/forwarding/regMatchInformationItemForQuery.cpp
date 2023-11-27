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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/common/orionldState.h"                         // orionldState, kjTreeLog
#include "orionld/regCache/RegCache.h"                           // RegCacheItem
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/forwarding/DistOp.h"                           // DistOp
#include "orionld/forwarding/distOpCreate.h"                     // distOpCreate
#include "orionld/forwarding/distOpListsMerge.h"                 // distOpListsMerge
#include "orionld/forwarding/regMatchEntityInfoForQuery.h"       // regMatchEntityInfoForQuery
#include "orionld/forwarding/regMatchAttributesForGet.h"         // regMatchAttributesForGet
#include "orionld/forwarding/regMatchInformationItemForQuery.h"  // Own interface



// -----------------------------------------------------------------------------
//
// attrsParam -
//
void attrsParam(DistOp* distOpP, StringArray* attrList, bool permanent)
{
  //
  // The attributes are in longnames but ... should probably compact them.
  // A registration can have its own @context, in cSourceInfo - for now, we use the @context of the original request.
  // The attrList is always cloned, so, no problem modifying it.
  //
  int attrsLen = 0;
  for (int ix = 0; ix < attrList->items; ix++)
  {
    attrList->array[ix]  = orionldContextItemAliasLookup(orionldState.contextP, attrList->array[ix], NULL, NULL);
    attrsLen            += strlen(attrList->array[ix]) + 1;
  }

  // Make room for "attrs=" and the string-end zero
  attrsLen += 7;

  char* attrs = (char*) ((permanent == true)? malloc(attrsLen) : kaAlloc(&orionldState.kalloc, attrsLen));

  if (attrs == NULL)
    LM_X(1, ("Out of memory"));

  bzero(attrs, attrsLen);

  strcpy(attrs, "attrs=");

  int   pos = 6;
  for (int ix = 0; ix < attrList->items; ix++)
  {
    int len = strlen(attrList->array[ix]);
    strcpy(&attrs[pos], attrList->array[ix]);

    // Add comma unless it's the last attr (in which case we add a zero, just in case)
    pos += len;

    if (ix != attrList->items - 1)  // Not the last attr
    {
      attrs[pos] = ',';
      pos += 1;
    }
    else
      attrs[pos] = 0;
  }

  distOpP->attrsParam    = attrs;
  distOpP->attrsParamLen = pos;
}



// -----------------------------------------------------------------------------
//
// regMatchInformationItemForQuery -
//
DistOp* regMatchInformationItemForQuery
(
  RegCacheItem* regP,
  KjNode*       infoP,
  StringArray*  idListP,
  StringArray*  typeListP,
  StringArray*  attrListP
)
{
  KjNode* entities   = kjLookup(infoP, "entities");
  DistOp* distOpList = NULL;

  if (entities != NULL)
  {
    DistOp* distOpP = NULL;

    for (KjNode* entityInfoP = entities->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
    {
      //
      // Creating a DistOp, in case entityInfoP matches (regMatchEntityInfoForQuery fills it in on hit).
      // If used:
      //   - distOpP is inserted in distOpList,
      //   - it is NULLed, and
      //   - it is allocated again in the next round of the loop
      //
      distOpP = distOpCreate(DoQueryEntity, regP, idListP, typeListP, attrListP);

      if (regMatchEntityInfoForQuery(regP, entityInfoP, idListP, typeListP, distOpP) == true)
      {
        if (distOpList == NULL)
          distOpList = distOpP;
        else
        {
          distOpP->next = distOpList;
          distOpList    = distOpP;
        }
      }
      else
        free(distOpP);  // FIXME: Allocate AFTER calling regMatchEntityInfoForQuery, not before!
    }

    if (distOpList == NULL)
      return NULL;
  }
  else
  {
    DistOp* distOpP = distOpCreate(DoQueryEntity, regP, NULL, typeListP, attrListP);

    if (distOpList == NULL)
      distOpList = distOpP;
    else
    {
      distOpP->next = distOpList;
      distOpList    = distOpP;
    }
  }

  //
  // The attributes are the same for all matching entity array items
  //
  KjNode*      propertyNamesP     = kjLookup(infoP, "propertyNames");
  KjNode*      relationshipNamesP = kjLookup(infoP, "relationshipNames");
  StringArray* attrUnionP         = regMatchAttributesForGet(regP, propertyNamesP, relationshipNamesP, attrListP, NULL);

  if (attrUnionP == NULL)
    return NULL;

  for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
  {
    distOpP->attrList = attrUnionP;
    if ((distOpP->attrList != NULL) && (distOpP->attrList->items > 0))
      attrsParam(distOpP, distOpP->attrList, true);
    else
      distOpP->attrsParam = NULL;
  }

  return distOpList;
}
