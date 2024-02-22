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

#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType
#include "orionld/types/OrionldRenderFormat.h"                   // OrionldRenderFormat
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/langStringExtract.h"                    // langValueFix
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/kjTree/kjAttributeNormalizedToSimplified.h"    // kjAttributeNormalizedToSimplified
#include "orionld/kjTree/kjAttributeNormalizedToConcise.h"       // kjAttributeNormalizedToConcise
#include "orionld/dbModel/dbModelToApiSubAttribute.h"            // dbModelToApiSubAttribute
#include "orionld/dbModel/dbModelToApiAttribute.h"               // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiAttribute - produce an NGSI-LD API Attribute from its DB format
//
void dbModelToApiAttribute(KjNode* dbAttrP, bool sysAttrs, bool eqsForDots)
{
  //
  // Remove unwanted parts of the attribute from DB
  //
  const char* unwanted[]   = { "mdNames", ".added", ".removed", "creDate",   "modDate"    };
  const char* ngsildName[] = { NULL,      NULL,     NULL,       "createdAt", "modifiedAt" };


  //
  // To truly be API Format, all '=' needs to be changed for '.'
  // But, if I do that here, then PATCH Entity2 stops working - it NEEDS the = for it's TREE thingy ...
  // Other service routines, like BATCH Upsert, need the eqForDot to be done, so ...
  //
  // if (eqsForDots == true)  // Only PATCH Entity2 sets eqsForDots == false - also doewsn't for for ~10 functests
  //   eqForDot(dbAttrP->name);

  for (unsigned int ix = 0; ix < K_VEC_SIZE(unwanted); ix++)
  {
    KjNode* nodeP = kjLookup(dbAttrP, unwanted[ix]);

    if (nodeP != NULL)
    {
      if ((sysAttrs == true) && (ix > 2))
      {
        nodeP->name = (char*) ngsildName[ix];

        if (dbAttrP->type == KjFloat)
        {
          char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
          numberToDate(dbAttrP->value.f, dateTimeBuf, 32);
          nodeP->value.s    = dateTimeBuf;
          nodeP->type       = KjString;
        }
      }
      else
        kjChildRemove(dbAttrP, nodeP);
    }
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
    if      (strcmp(typeP->value.s, "Relationship")       == 0) valueP->name = (char*) "object";
    else if (strcmp(typeP->value.s, "LanguageProperty")   == 0) valueP->name = (char*) "languageMap";
    else if (strcmp(typeP->value.s, "JsonProperty")       == 0) valueP->name = (char*) "json";
    else if (strcmp(typeP->value.s, "VocabularyProperty") == 0)
    {
      valueP->name = (char*) "vocab";

      if (valueP->type == KjString)
        valueP->value.s = orionldContextItemAliasLookup(orionldState.contextP, valueP->value.s, NULL, NULL);
      else if (valueP->type == KjArray)
      {
        for (KjNode* wordP = valueP->value.firstChildP; wordP != NULL; wordP = wordP->next)
        {
          if (wordP->type == KjString)
            wordP->value.s = orionldContextItemAliasLookup(orionldState.contextP, wordP->value.s, NULL, NULL);
        }
      }
    }
  }


  //
  // Sub-Attributes
  //
  KjNode* mdP = kjLookup(dbAttrP, "md");
  if (mdP != NULL)
  {
    kjChildRemove(dbAttrP, mdP);  // The content of mdP is added to dbAttrP at the end of the if

    //
    // Treating sub-attributes
    //
    // Special attention to: unitCode + observedAt
    //
    for (KjNode* subP = mdP->value.firstChildP; subP != NULL; subP = subP->next)
    {
      if (strcmp(subP->name, "unitCode") == 0)
      {
        subP->type  = subP->value.firstChildP->type;
        subP->value = subP->value.firstChildP->value;
      }
      else if (strcmp(subP->name, "observedAt") == 0)  // Part of Sub-Attribute
      {
        char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
        numberToDate(subP->value.firstChildP->value.f, dateTimeBuf, 32);
        subP->type     = KjString;
        subP->value.s  = dateTimeBuf;
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
// sysAttrsStrip -
//
static void sysAttrsStrip(KjNode* attrP)
{
  KjNode* createdAtP  = kjLookup(attrP, "createdAt");
  KjNode* modifiedAtP = kjLookup(attrP, "modifiedAt");

  if (createdAtP != NULL)
    kjChildRemove(attrP, createdAtP);
  if (modifiedAtP != NULL)
    kjChildRemove(attrP, modifiedAtP);
}



// -----------------------------------------------------------------------------
//
// sysAttrsToTimestamps -
//
static void sysAttrsToTimestamps(KjNode* attrP)
{
  KjNode* createdAtP  = kjLookup(attrP, "createdAt");
  KjNode* modifiedAtP = kjLookup(attrP, "modifiedAt");

  if (createdAtP != NULL)
  {
    char* createdAtBuf = kaAlloc(&orionldState.kalloc, 32);
    numberToDate(createdAtP->value.f, createdAtBuf, 32);
    createdAtP->type       = KjString;
    createdAtP->value.s    = createdAtBuf;
  }

  if (modifiedAtP != NULL)
  {
    char* modifiedAtBuf = kaAlloc(&orionldState.kalloc, 32);
    numberToDate(modifiedAtP->value.f, modifiedAtBuf, 32);
    modifiedAtP->type       = KjString;
    modifiedAtP->value.s    = modifiedAtBuf;
  }
}



// -----------------------------------------------------------------------------
//
// dbModelToApiLangPropertySimplified -
//
void dbModelToApiLangPropertySimplified(KjNode* dbAttrP, const char* lang)
{
  KjNode* dbValueP = kjLookup(dbAttrP, "value");
  KjNode* dbTypeP  = kjLookup(dbAttrP, "type");

  if (lang != NULL)
    langValueFix(dbAttrP, dbValueP, dbTypeP, lang);
  else
  {
    dbValueP->name = (char*) "languageMap";

    // Remove everything except dbValueP (languageMap)
    dbAttrP->value.firstChildP = dbValueP;
    dbAttrP->lastChild         = dbValueP;
    dbValueP->next = NULL;
  }
}



// -----------------------------------------------------------------------------
//
// dbModelToApiAttribute2 -
//
KjNode* dbModelToApiAttribute2(KjNode* dbAttrP, KjNode* datasetP, bool sysAttrs, OrionldRenderFormat renderFormat, const char* lang, bool compacted, OrionldProblemDetails* pdP)
{
  if ((renderFormat == RF_CROSS_APIS_NORMALIZED) || (renderFormat == RF_CROSS_APIS_SIMPLIFIED))
    compacted = false;

  if (datasetP != NULL)
  {
    char* shortName = datasetP->name;

    if (compacted == true)
    {
      char* longName  = (compacted == true)? kaStrdup(&orionldState.kalloc, datasetP->name) : datasetP->name;
      eqForDot(longName);
      shortName = orionldContextItemAliasLookup(orionldState.contextP, longName, NULL, NULL);
    }
    else
      eqForDot(shortName);

    KjNode* attrArray = kjArray(orionldState.kjsonP, shortName);

    // 1. First the Default attribute
    if (dbAttrP != NULL)
    {
      KjNode* apiAttrP = dbModelToApiAttribute2(dbAttrP, NULL, sysAttrs, renderFormat, lang, compacted, pdP);

      if (apiAttrP == NULL)
        return NULL;
      kjChildAdd(attrArray, apiAttrP);
    }

    // 2. datasets
    //    - The datasets are not stored in the NGSIv1 database model.
    //      As this is new, I could pick any database model and I picked a model identical to the API datamodel, of course!
    //      No transformation is needed, just add the attr to its array.l
    //
    if (datasetP->type == KjObject)  // just one
    {
      if (renderFormat == RF_CONCISE)
        kjAttributeNormalizedToConcise(datasetP, orionldState.uriParams.lang);
      else if (renderFormat == RF_SIMPLIFIED)
        kjAttributeNormalizedToSimplified(datasetP, orionldState.uriParams.lang);

      kjChildAdd(attrArray, datasetP);  // Was removed from @datasets by caller (

      if (sysAttrs == false)
        sysAttrsStrip(datasetP);
      else
        sysAttrsToTimestamps(datasetP);
    }
    else if (datasetP->type == KjArray)  // Many
    {
      dbAttrP = datasetP->value.firstChildP;
      KjNode* next;
      while (dbAttrP != NULL)
      {
        next = dbAttrP->next;
        dbAttrP->name = datasetP->name;
        kjChildRemove(datasetP, dbAttrP);

        kjChildAdd(attrArray, dbAttrP);

        if (sysAttrs == false)
          sysAttrsStrip(dbAttrP);
        else
          sysAttrsToTimestamps(dbAttrP);

        if (renderFormat == RF_CONCISE)
          kjAttributeNormalizedToConcise(dbAttrP, orionldState.uriParams.lang);
        else if (renderFormat == RF_SIMPLIFIED)
          kjAttributeNormalizedToSimplified(dbAttrP, orionldState.uriParams.lang);

        dbAttrP = next;
      }
    }

    if ((attrArray->value.firstChildP != NULL) && (attrArray->value.firstChildP->next == NULL))  // One single item in the array
    {
      attrArray->value.firstChildP->name = shortName;
      return attrArray->value.firstChildP;
    }

    return attrArray;
  }

  char* shortName = dbAttrP->name;

  if (compacted == true)
  {
    char*   longName = kaStrdup(&orionldState.kalloc, dbAttrP->name);

    eqForDot(longName);
    shortName = orionldContextItemAliasLookup(orionldState.contextP, longName, NULL, NULL);
  }
  else
    eqForDot(shortName);

  KjNode* attrP = NULL;

  //
  // If CONCISE:
  //   - and Property/GeoProperty
  //   - and No metadata
  // => KeyValues
  //
  // Else, just remove the attribute type, keep value/object/languageMap/vocab
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

  if ((renderFormat == RF_SIMPLIFIED) || (conciseAsKeyValues == true))
  {
    if (attrTypeNodeP == NULL)
      attrTypeNodeP = kjLookup(dbAttrP, "type");

    if (strcmp(attrTypeNodeP->value.s, "LanguageProperty") == 0)
    {
      dbModelToApiLangPropertySimplified(dbAttrP, lang);
      attrP = dbAttrP;
    }
    else if (strcmp(attrTypeNodeP->value.s, "VocabularyProperty") == 0)
    {
      KjNode* valueP = kjLookup(dbAttrP, "value");

      if (valueP->type == KjString)
        valueP->value.s = orionldContextItemAliasLookup(orionldState.contextP, valueP->value.s, NULL, NULL);
      else if (valueP->type == KjArray)
      {
        for (KjNode* wordP = valueP->value.firstChildP; wordP != NULL; wordP = wordP->next)
        {
          if (wordP->type == KjString)
            wordP->value.s = orionldContextItemAliasLookup(orionldState.contextP, wordP->value.s, NULL, NULL);
        }
      }

      // Remove everything except the value, and change its name to "vocab"
      dbAttrP->value.firstChildP = valueP;
      dbAttrP->lastChild         = valueP;
      valueP->next               = NULL;
      valueP->name               = (char*) "vocab";
      attrP = dbAttrP;
    }
    else if (strcmp(attrTypeNodeP->value.s, "JsonProperty") == 0)
    {
      KjNode* valueP = kjLookup(dbAttrP, "value");

      // Remove everything except the value, and change its name to "vocab"
      dbAttrP->value.firstChildP = valueP;
      dbAttrP->lastChild         = valueP;
      valueP->next               = NULL;
      valueP->name               = (char*) "json";
      attrP = dbAttrP;
    }
    else
    {
      // "Steal" the value node and rename it to have the attribute's name instead - that's all that's needed for SIMPLIFIED FORMAT
      attrP = kjLookup(dbAttrP, "value");  // In the DB, all attributes have the "value" name.
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

    KjNode* nodeP = dbAttrP->value.firstChildP;
    KjNode* next;

    while (nodeP != NULL)
    {
      next = nodeP->next;

      if (strcmp(nodeP->name, "value") == 0)
      {
        if (attrType == Relationship)
          nodeP->name = (char*) "object";
        else if (attrType == JsonProperty)
          nodeP->name = (char*) "json";
        else if (attrType == VocabularyProperty)
        {
          nodeP->name = (char*) "vocab";

          if (nodeP->type == KjString)
            nodeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, nodeP->value.s, NULL, NULL);
          else if (nodeP->type == KjArray)
          {
            for (KjNode* wordP = nodeP->value.firstChildP; wordP != NULL; wordP = wordP->next)
            {
              if (wordP->type == KjString)
                wordP->value.s = orionldContextItemAliasLookup(orionldState.contextP, wordP->value.s, NULL, NULL);
            }
          }
        }
        else if (attrType == LanguageProperty)
        {
          if (lang != NULL)
          {
            // FIXME: Try to use langValueFix
            KjNode* langNodeP = kjLookup(nodeP, lang);

            if (renderFormat == RF_NORMALIZED)  // For CONCISE the attribute type is not present
              typeP->value.s = (char*) "Property";

            if (langNodeP == NULL)
              langNodeP = kjLookup(nodeP, "@none");  // Try @none if not found

            if (langNodeP == NULL)
              langNodeP = kjLookup(nodeP, "en");  // Try English if not found

            if (langNodeP == NULL)
              langNodeP = nodeP->value.firstChildP;  // Take the first one if English is also not found

            if (langNodeP != NULL)
            {
              KjNode* langP = kjString(orionldState.kjsonP, "lang", langNodeP->name);
              kjChildAdd(attrP, langP);

              if (langNodeP->type == KjString)
              {
                nodeP->type    = KjString;
                nodeP->value.s = langNodeP->value.s;
              }
              else if (langNodeP->type == KjArray)
              {
                nodeP->type                   = KjArray;
                nodeP->value.firstChildP      = langNodeP->value.firstChildP;
                nodeP->lastChild              = langNodeP->lastChild;

                langNodeP->value.firstChildP  = NULL;
                langNodeP->lastChild          = NULL;
              }
            }
            else
            {
              nodeP->type    = KjString;
              nodeP->value.s = (char*) "empty languageMap";
            }
          }
          else
            nodeP->name = (char*) "languageMap";
        }

        kjChildRemove(dbAttrP, nodeP);
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

          kjChildRemove(dbAttrP, nodeP);
          kjChildAdd(attrP, nodeP);
        }
      }
      else if (strcmp(nodeP->name, "md") == 0)
        mdsP = nodeP;

      nodeP = next;
    }

    if (mdsP != NULL)
    {
      KjNode* mdP = mdsP->value.firstChildP;
      KjNode* next;

      while (mdP != NULL)
      {
        KjNode* subAttributeP;

        next = mdP->next;

        if ((subAttributeP = dbModelToApiSubAttribute2(mdP, sysAttrs, renderFormat, lang, pdP)) == NULL)
        {
          LM_E(("Datamodel Error (%s: %s)", pdP->title, pdP->detail));
          return NULL;
        }

        kjChildRemove(mdsP, mdP);
        kjChildAdd(attrP, subAttributeP);
        mdP = next;
      }
    }
  }

  return attrP;
}
