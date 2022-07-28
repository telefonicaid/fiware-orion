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
#include <string.h>                                                 // strcmp
#include <unistd.h>                                                 // NULL

extern "C"
{
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjLookup.h"                                         // kjLookup
#include "kjson/kjBuilder.h"                                        // kjChildRemove, ...
}

#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/common/langStringExtract.h"                       // langStringExtract
#include "orionld/kjTree/kjChildCount.h"                            // kjChildCount
#include "orionld/kjTree/kjEntityNormalizedToSimplified.h"          // Own interface



// ----------------------------------------------------------------------------
//
// kjEntityNormalizedToSimplified -
//
// FIXME: Fix kjAttributeNormalizedToSimplified (from here) and call that function
//
void kjEntityNormalizedToSimplified(KjNode* treeP, const char* lang)
{
  for (KjNode* attrP = treeP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type != KjObject)
      continue;

    KjNode* typeP  = kjLookup(attrP, "type");
    KjNode* valueP = NULL;

    if (typeP == NULL)
    {
      LM_E(("Database Error (no type found for attibute '%s')", attrP->name));
      continue;
    }

    if ((strcmp(typeP->value.s, "Property") == 0) || (strcmp(typeP->value.s, "GeoProperty") == 0))
      valueP = kjLookup(attrP, "value");
    else if (strcmp(typeP->value.s, "Relationship") == 0)
      valueP = kjLookup(attrP, "object");
    else if (strcmp(typeP->value.s, "LanguageProperty") == 0)
    {
      valueP = kjLookup(attrP, "languageMap");

      if (lang != NULL)
      {
        char* pickedLanguage;
        //
        // FIXME
        // langStringExtract should not be used - the RHS might be an Array
        // Somehow this works anyweay. If Array, we don't enter here and langStringExtract
        // is only called if RHS is a String.
        // Need to understand this and clean it up
        //
        char* langString = langStringExtract(valueP, lang, &pickedLanguage);  // FIXME: use langItemPick instead - can be array as well !

        valueP->type     = KjString;
        valueP->value.s  = langString;
      }
    }

    if (valueP != NULL)
    {
      attrP->type  = valueP->type;
      attrP->value = valueP->value;
    }
    else
      LM_E(("Database Error (no value/object/languageMap found for attribute '%s')", attrP->name));
  }
}
