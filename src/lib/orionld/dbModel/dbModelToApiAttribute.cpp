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
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjString
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "common/RenderFormat.h"                                 // RenderFormat

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/dbModel/dbModelToApiSubAttribute.h"            // dbModelToApiSubAttribute
#include "orionld/dbModel/dbModelToApiAttribute.h"               // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiAttribute - produce an NGSI-LD API Attribute from its DB format
//
void dbModelToApiAttribute(KjNode* dbAttrP, bool sysAttrs)
{
  LM_TMP(("**************************** IN dbModelToApiAttribute for '%s'", dbAttrP->name));
  //
  // Remove unwanted parts of the attribute from DB
  //
  const char* unwanted[]   = { "mdNames", "creDate",   "modDate" };
  const char* ngsildName[] = { NULL,      "createdAt", "modifiedAt" };

  for (unsigned int ix = 0; ix < K_VEC_SIZE(unwanted); ix++)
  {
    KjNode* nodeP = kjLookup(dbAttrP, unwanted[ix]);

    if (nodeP != NULL)
    {
      if ((sysAttrs == true) && (ix > 0))
      {
        char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
        numberToDate(dbAttrP->value.f, dateTimeBuf, 32);
        dbAttrP->name       = (char*) ngsildName[ix];
        dbAttrP->value.s    = dateTimeBuf;
        dbAttrP->type       = KjString;
      }
      else
        kjChildRemove(dbAttrP, nodeP);
    }
  }

  KjNode* observedAtP = kjLookup(dbAttrP, "observedAt");
  KjNode* unitCodeP   = kjLookup(dbAttrP, "unitCode");

  if (observedAtP != NULL)
  {
    char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
    numberToDate(observedAtP->value.firstChildP->value.f, dateTimeBuf, 32);
    observedAtP->type      = KjString;
    observedAtP->value.s   = dateTimeBuf;
  }

  if (unitCodeP != NULL)
  {
    unitCodeP->type      = KjString;
    unitCodeP->value.s   = unitCodeP->value.firstChildP->value.s;
    unitCodeP->lastChild = NULL;
  }

  KjNode* typeP  = kjLookup(dbAttrP, "type");
  KjNode* valueP = kjLookup(dbAttrP, "value");

  if (typeP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (attribute without type in database)", dbAttrP->name, 500);
    return;
  }

  if (valueP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (attribute without value in database)", dbAttrP->name, 500);
    return;
  }

  if ((typeP != NULL) && (valueP != NULL) && (typeP->type == KjString))
  {
    if      (strcmp(typeP->value.s, "Relationship")     == 0) valueP->name = (char*) "object";
    else if (strcmp(typeP->value.s, "LanguageProperty") == 0) valueP->name = (char*) "languageMap";
  }


  //
  // Sub-Attributes
  //
  KjNode* mdP = kjLookup(dbAttrP, "md");
  if (mdP != NULL)
  {
    kjChildRemove(dbAttrP, mdP);  // The content of mdP is added to dbAttrP at the end of the if

    // Special Sub-Attrs: unitCode + observedAt
    for (KjNode* subP = mdP->value.firstChildP; subP != NULL; subP = subP->next)
    {
      if (strcmp(subP->name, "unitCode") == 0)
      {
        subP->type  = subP->value.firstChildP->type;
        subP->value = subP->value.firstChildP->value;
      }
      else if (strcmp(subP->name, "observedAt") == 0)  // Part of Sub-Attribute
      {
        subP->type  = subP->value.firstChildP->type;
        subP->value = subP->value.firstChildP->value;  // + convert to iso8601 string
      }
      else
        dbModelToApiSubAttribute(subP);
    }

    //
    // Move all metadata (sub-attrs) up one level
    //
    dbAttrP->lastChild->next = mdP->value.firstChildP;
    dbAttrP->lastChild       = mdP->lastChild;
  }
}



// -----------------------------------------------------------------------------
//
// dbModelToApiAttribute2 -
//
KjNode* dbModelToApiAttribute2(KjNode* dbAttrP, bool sysAttrs, RenderFormat renderFormat, char* lang, OrionldProblemDetails* pdP)
{
  char*   longName = kaStrdup(&orionldState.kalloc, dbAttrP->name);

  eqForDot(longName);

  char*   shortName = orionldContextItemAliasLookup(orionldState.contextP, longName, NULL, NULL);
  KjNode* attrP     = kjObject(orionldState.kjsonP, shortName);
  KjNode* mdsP      = NULL;
  KjNode* typeP     = kjLookup(dbAttrP, "type");

  if (typeP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (attribute without type in database)", dbAttrP->name, 500);
    return NULL;
  }

  OrionldAttributeType attrType = orionldAttributeType(typeP->value.s);
  kjChildRemove(dbAttrP, typeP);
  kjChildAdd(attrP, typeP);

  KjNode* nodeP = dbAttrP->value.firstChildP;
  KjNode* next;
  while (nodeP != NULL)
  {
    next = nodeP->next;

    if (strcmp(nodeP->name, "value") == 0)
    {
      if      (attrType == Relationship)      nodeP->name = (char*) "object";
      else if (attrType == LanguageProperty)  nodeP->name = (char*) "languageMap";

      kjChildAdd(attrP, nodeP);
    }
    else if (sysAttrs == true)
    {
      if (strcmp(nodeP->name, "creDate") == 0)
      {
        nodeP->name = (char*) "createdAt";
        kjChildAdd(attrP, nodeP);
      }
      else if (strcmp(nodeP->name, "modDate") == 0)
      {
        nodeP->name = (char*) "modifiedAt";
        kjChildAdd(attrP, nodeP);
      }
    }
    else if (strcmp(nodeP->name, "md") == 0)
      mdsP = nodeP;

    nodeP = next;
  }

  if (mdsP != NULL)
  {
    for (KjNode* mdP = mdsP->value.firstChildP; mdP != NULL; mdP = mdP->next)
    {
      KjNode* subAttributeP;

      if ((subAttributeP = dbModelToApiSubAttribute2(mdP, sysAttrs, renderFormat, lang, pdP)) == NULL)
      {
        LM_E(("Datamodel Error (%s: %s)", pdP->title, pdP->detail));
        return NULL;
      }

      kjChildAdd(attrP, subAttributeP);
    }
  }

  return attrP;
}
