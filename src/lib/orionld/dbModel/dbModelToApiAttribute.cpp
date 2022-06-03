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
  LM_TMP(("Attribute '%s'", dbAttrP->name));
  char*   longName = kaStrdup(&orionldState.kalloc, dbAttrP->name);
  eqForDot(longName);
  char*   shortName = orionldContextItemAliasLookup(orionldState.contextP, longName, NULL, NULL);
  KjNode* attrP     = NULL;

  //
  // If CONCISE:
  //   - and Property/GeoProperty
  //   - and No metadata
  // => KeyValues
  //
  // Else, just remove the attribute type, keep value/object/languageMap
  // And call dbModelToApiSubAttribute2 with Concise
  //
  bool    conciseAsKeyValues = false;
  KjNode* attrTypeNodeP      = NULL;

  if ((renderFormat == RF_CONCISE) && (sysAttrs == false))
  {
    attrTypeNodeP = kjLookup(dbAttrP, "type");
    if ((strcmp(attrTypeNodeP->value.s, "Property") == 0) || (strcmp(attrTypeNodeP->value.s, "GeoProperty") == 0))
    {
      KjNode* mdP = kjLookup(dbAttrP, "md");
      if ((mdP == NULL) || (mdP->value.firstChildP == NULL))  // No sub-attributes
        conciseAsKeyValues = true;
    }
  }
  LM_TMP(("conciseAsKeyValues: %s", (conciseAsKeyValues == true)? "true" : "false"));

  if ((renderFormat == RF_KEYVALUES) || (conciseAsKeyValues == true))
  {
    // "Steal" the value node and rename it to have the attribute's name instead - that's all that's needed for SIMPLIFIED FORMAT
    attrP = kjLookup(dbAttrP, "value");

    if (attrTypeNodeP == NULL)
      attrTypeNodeP = kjLookup(dbAttrP, "type");

    if ((lang != NULL) && (strcmp(attrTypeNodeP->value.s, "LanguageProperty") == 0))
    {
      KjNode* langP = kjLookup(attrP, lang);
      if (langP == NULL)
        langP = kjLookup(attrP, "en");
      if (langP == NULL)
        langP = attrP->value.firstChildP;

      attrP->type = KjString;
      attrP->value.s = (langP != NULL)? langP->value.s : (char*) "empty languageMap";
    }

    attrP->name = shortName;
  }
  else  // RF_NORMALIZED  or  RF_CONCISE
  {
    KjNode* mdsP    = NULL;
    KjNode* typeP   = (attrTypeNodeP == NULL)? kjLookup(dbAttrP, "type") : attrTypeNodeP;

    attrP = kjObject(orionldState.kjsonP, shortName);
    if (typeP == NULL)
    {
      orionldError(OrionldInternalError, "Database Error (attribute without type in database)", dbAttrP->name, 500);
      return NULL;
    }

    OrionldAttributeType attrType = orionldAttributeType(typeP->value.s);
    kjChildRemove(dbAttrP, typeP);

    if (renderFormat == RF_NORMALIZED)  // For CONCISE we don't want the attribute type
      kjChildAdd(attrP, typeP);
    else
      LM_TMP(("Not adding attr type as CONCISE format"));

    KjNode* nodeP = dbAttrP->value.firstChildP;
    KjNode* next;
    LM_TMP(("Looping over attribute parts"));
    while (nodeP != NULL)
    {
      LM_TMP(("Attribute part '%s'", nodeP->name));
      next = nodeP->next;

      if (strcmp(nodeP->name, "value") == 0)
      {
        if (attrType == Relationship)
          nodeP->name = (char*) "object";
        else if (attrType == LanguageProperty)
        {
          LM_TMP(("LANG: Got a languageMap"));
          LM_TMP(("LANG: lang: %s", lang));

          if (lang != NULL)
          {
            LM_TMP(("LANG: Need to look up '%s' in the languageMap", lang));
            KjNode* langNodeP = kjLookup(nodeP, lang);

            if (renderFormat == RF_NORMALIZED)  // For CONCISE the attribute type is not present
              typeP->value.s = (char*) "Property";

            if (langNodeP != NULL)
              LM_TMP(("Found wanted language '%s'. Value: '%s'", langNodeP->name, langNodeP->value.s));

            if (langNodeP == NULL)
            {
              LM_TMP(("wanted language '%s' not found - looking up English instead", lang));
              langNodeP = kjLookup(nodeP, "en");  // Try English if not found
            }

            if (langNodeP == NULL)
            {
              LM_TMP(("Nor wanted language '%s' nor English found - taking first child", lang));
              langNodeP = nodeP->value.firstChildP;  // Take the first one if English is also not found
            }

            if (langNodeP != NULL)
            {
              LM_TMP(("Time to change type of the languageMap, and add 'lang': '%s'", langNodeP->name));
              KjNode* langP = kjString(orionldState.kjsonP, "lang", langNodeP->name);
              kjChildAdd(attrP, langP);

              nodeP->type    = KjString;
              nodeP->value.s = langNodeP->value.s;
            }
            else
            {
              LM_TMP(("empty languageMap"));
              nodeP->type    = KjString;
              nodeP->value.s = (char*) "empty languageMap";
            }
          }
          else
            nodeP->name = (char*) "languageMap";
        }

        kjChildAdd(attrP, nodeP);
      }
      else if ((strcmp(nodeP->name, "creDate") == 0) || (strcmp(nodeP->name, "modDate") == 0))
      {
        if (sysAttrs == true)
        {
          nodeP->name = (nodeP->name[0] == 'c')? (char*) "createdAt" : (char*) "modifiedAt";

          char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
          numberToDate(nodeP->value.f, dateTimeBuf, 32);
          nodeP->type      = KjString;
          nodeP->value.s   = dateTimeBuf;

          kjChildAdd(attrP, nodeP);
        }
      }
      else if (strcmp(nodeP->name, "md") == 0)
      {
        mdsP = nodeP;
        LM_TMP(("Found 'md' (%p) - there are sub-attrs", mdsP));
      }

      nodeP = next;
    }

    if (mdsP != NULL)
    {
      LM_TMP(("Looping over sub-attributes"));
      for (KjNode* mdP = mdsP->value.firstChildP; mdP != NULL; mdP = mdP->next)
      {
        LM_TMP(("Sub-Attribute '%s' (sysAttrs: %s)", mdP->name, (sysAttrs == true)? "true" : "false"));
        KjNode* subAttributeP;

        if ((subAttributeP = dbModelToApiSubAttribute2(mdP, sysAttrs, renderFormat, lang, pdP)) == NULL)
        {
          LM_E(("Datamodel Error (%s: %s)", pdP->title, pdP->detail));
          return NULL;
        }

        kjChildAdd(attrP, subAttributeP);
      }
    }
  }

  return attrP;
}
