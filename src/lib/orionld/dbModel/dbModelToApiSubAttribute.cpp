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
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove. kjString, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType
#include "orionld/types/OrionldRenderFormat.h"                   // OrionldRenderFormat
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/dbModel/dbModelToObservedAt.h"                 // dbModelToObservedAt
#include "orionld/dbModel/dbModelToUnitCode.h"                   // dbModelToUnitCode
#include "orionld/dbModel/dbModelToApiSubAttribute.h"            // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiSubAttribute -
//
void dbModelToApiSubAttribute(KjNode* dbSubAttrP)
{
  //
  // Remove unwanted parts of the sub-attribute from DB
  //
  const char* unwanted[] = { "createdAt", "modifiedAt" };

  for (unsigned int ix = 0; ix < K_VEC_SIZE(unwanted); ix++)
  {
    KjNode* nodeP = kjLookup(dbSubAttrP, unwanted[ix]);

    if (nodeP != NULL)
      kjChildRemove(dbSubAttrP, nodeP);
  }

  KjNode* typeP  = kjLookup(dbSubAttrP, "type");
  KjNode* valueP = kjLookup(dbSubAttrP, "value");

  if (typeP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (sub-attribute without type in database)", dbSubAttrP->name, 500);
    return;
  }

  if (valueP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (attribute without value in database)", dbSubAttrP->name, 500);
    return;
  }

  if ((typeP != NULL) && (valueP != NULL) && (typeP->type == KjString))
  {
    if      (strcmp(typeP->value.s, "Relationship")       == 0) valueP->name = (char*) "object";
    else if (strcmp(typeP->value.s, "LanguageProperty")   == 0) valueP->name = (char*) "languageMap";
    else if (strcmp(typeP->value.s, "VocabularyProperty") == 0) valueP->name = (char*) "vocab";
    else if (strcmp(typeP->value.s, "JsonProperty")       == 0) valueP->name = (char*) "json";
  }
}



// -----------------------------------------------------------------------------
//
// dbModelToApiSubAttribute2 - transform a sub-attribute from DB Model to API format
//
KjNode* dbModelToApiSubAttribute2(KjNode* dbSubAttributeP, bool sysAttrs, OrionldRenderFormat renderFormat, const char* lang, OrionldProblemDetails* pdP)
{
  if (strcmp(dbSubAttributeP->name, "observedAt") == 0)
    return dbModelToObservedAt(dbSubAttributeP);
  else if (strcmp(dbSubAttributeP->name, "unitCode") == 0)
    return dbModelToUnitCode(dbSubAttributeP);

  char*   longName = kaStrdup(&orionldState.kalloc, dbSubAttributeP->name);
  eqForDot(longName);

  char*   alias    = orionldContextItemAliasLookup(orionldState.contextP, longName, NULL, NULL);
  KjNode* subAttrP = kjObject(orionldState.kjsonP, alias);
  KjNode* typeP    = kjLookup(dbSubAttributeP, "type");

  if (typeP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (sub-attribute without type in database)", dbSubAttributeP->name, 500);
    return NULL;
  }

  OrionldAttributeType subAttrType = orionldAttributeType(typeP->value.s);
  kjChildRemove(dbSubAttributeP, typeP);


  if (renderFormat == RF_CONCISE)
  {
    // Might be key-values
    if ((sysAttrs == false) && ((subAttrType == Property) || (subAttrType == GeoProperty)))
    {
      KjNode* valueP = kjLookup(dbSubAttributeP, "value");

      kjChildRemove(dbSubAttributeP, valueP);
      valueP->name = alias;
      return valueP;
    }
  }
  else
    kjChildAdd(subAttrP, typeP);  // No "type" if CONCISE

  KjNode* nodeP = dbSubAttributeP->value.firstChildP;
  KjNode* next;
  while (nodeP != NULL)
  {
    next = nodeP->next;

    if (strcmp(nodeP->name, "value") == 0)
    {
      if      (subAttrType == Relationship)        nodeP->name = (char*) "object";
      else if (subAttrType == LanguageProperty)    nodeP->name = (char*) "languageMap";
      else if (subAttrType == VocabularyProperty)  nodeP->name = (char*) "vocab";
      else if (subAttrType == JsonProperty)        nodeP->name = (char*) "json";

      kjChildAdd(subAttrP, nodeP);
    }
    else if (strcmp(nodeP->name, "observedAt") == 0)
      kjChildAdd(subAttrP, nodeP);
    else if (strcmp(nodeP->name, "unitCode") == 0)
      kjChildAdd(subAttrP, nodeP);
    else if (sysAttrs == true)
    {
      if ((strcmp(nodeP->name, "createdAt") == 0) || (strcmp(nodeP->name, "modifiedAt") == 0))
      {
        char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
        numberToDate(nodeP->value.f, dateTimeBuf, 32);
        nodeP->value.s    = dateTimeBuf;
        nodeP->type       = KjString;

        kjChildAdd(subAttrP, nodeP);
      }
    }

    nodeP = next;
  }

  return subAttrP;
}
