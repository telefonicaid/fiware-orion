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
#include <string.h>                                            // strcmp
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildRemove, ...
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/langStringExtract.h"                  // langStringExtract
#include "orionld/kjTree/kjChildCount.h"                       // kjChildCount
#include "orionld/kjTree/kjEntityNormalizedToConcise.h"        // Own interface



// ----------------------------------------------------------------------------
//
// kjEntityNormalizedToConcise -
//
// FIXME: Fix kjAttributeNormalizedToConcise (from here) and call that function
//
void kjEntityNormalizedToConcise(KjNode* treeP, const char* lang)
{
  for (KjNode* attrP = treeP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type != KjObject)
      continue;

    if ((treeP->name != NULL) && (strcmp(treeP->name, "value") == 0))
      continue;

    KjNode* typeP  = kjLookup(attrP, "type");
    KjNode* valueP = NULL;

    if (typeP == NULL)
    {
      LM_E(("Database Error (no type found for attribute '%s')", attrP->name));
      continue;
    }

    bool typeDeduced = false;
    if ((strcmp(typeP->value.s, "Property") == 0) || (strcmp(typeP->value.s, "GeoProperty") == 0))
    {
      valueP = kjLookup(attrP, "value");
      typeDeduced = true;
    }
    else if (strcmp(typeP->value.s, "Relationship") == 0)
      typeDeduced = true;
    else if (strcmp(typeP->value.s, "LanguageProperty") == 0)
    {
      typeDeduced = true;
      valueP = kjLookup(attrP, "languageMap");

      if (lang != NULL)
      {
        //
        // type is modified from LanguageProperty to Property, and then removed as we're in Concise, so, no action needed for that
        // The value (not languageMap anymore is a string, looked up by langStringExtract
        //
        char* pickedLanguage;
        char* langString = langStringExtract(valueP, lang, &pickedLanguage);  // FIXME: use langItemPick instead - can be array as well !

        valueP->name    = (char*) "value";
        valueP->type    = KjString;
        valueP->value.s = langString;

        // Add 'lang' sub-attr
        KjNode* langP = kjString(orionldState.kjsonP, "lang", pickedLanguage);
        kjChildAdd(attrP, langP);
      }
      else
        valueP = NULL;  // To avoid key-values
    }

    // Remove 'type' - for attributes and sub-attributes only
    if (typeDeduced == true)
      kjChildRemove(attrP, typeP);

    // If only value - make it keyValues - only valid for Property (or LangProperty + lang that made it a Property
    if ((valueP != NULL) && (kjChildCount(attrP) == 1))
    {
      attrP->type      = valueP->type;
      attrP->value     = valueP->value;
      continue;
    }

    // Same same for Sub-Attributes
    kjEntityNormalizedToConcise(attrP, lang);
  }
}
