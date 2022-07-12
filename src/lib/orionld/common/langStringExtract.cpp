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
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjString
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/langStringExtract.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// langStringExtract -
//
// FIXME: merge this function with langValueFix
//
char* langStringExtract(KjNode* languageMapP, const char* lang, char** pickedLanguageP)
{
  KjNode* langItemP = kjLookup(languageMapP, lang);  // Find the desired language item (key == lang) inside the languageMap

  if (langItemP == NULL)
    langItemP = kjLookup(languageMapP, "@none");  // If present, the key '@none' is the default language
  if (langItemP == NULL)
    langItemP = kjLookup(languageMapP, "en");     // English is the default if 'lang' is not present and @none also not
  if (langItemP == NULL)
    langItemP = languageMapP->value.firstChildP;  // If English also not present, just pick the first one

  *pickedLanguageP = NULL;
  if (langItemP == NULL)
    return (char*) "empty languageMap";  // If there is no item at all inside the language map, use "empty languageMap"

  *pickedLanguageP = langItemP->name;
  return langItemP->value.s;
}



// -----------------------------------------------------------------------------
//
// langItemPick -
//
KjNode* langItemPick(KjNode* languageMapP, const char* attrName, const char* lang, char** pickedLanguageP)
{
  *pickedLanguageP = NULL;

  KjNode* langItemP = kjLookup(languageMapP, lang);  // Find the desired language item (key == lang) inside the languageMap

  if (langItemP == NULL)
    langItemP = kjLookup(languageMapP, "@none");  // If present, the key '@none' is the default language
  if (langItemP == NULL)
    langItemP = kjLookup(languageMapP, "en");     // English is the default if 'lang' is not present and @none also not
  if (langItemP == NULL)
    langItemP = languageMapP->value.firstChildP;  // If English also not present, just pick the first one
  if (langItemP == NULL)                          // If there is no item at all inside the language map, use "empty languageMap"
    return kjString(orionldState.kjsonP, attrName, "empty languageMap");

  *pickedLanguageP = langItemP->name;

  langItemP->name = kaStrdup(&orionldState.kalloc, attrName);
  return langItemP;
}
