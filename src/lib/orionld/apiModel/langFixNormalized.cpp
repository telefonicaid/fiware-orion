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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd, kjString
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/apiModel/langFixNormalized.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// langFixNormalized -
//
void langFixNormalized(KjNode* attrP, KjNode* typeP, KjNode* languageMapP, const char* lang)
{
  KjNode* valueP = kjLookup(languageMapP, lang);
  if (valueP == NULL)
    valueP = kjLookup(languageMapP, "@none");
  if (valueP == NULL)
    valueP = kjLookup(languageMapP, "en");
  if (valueP == NULL)
    valueP = languageMapP->value.firstChildP;

  if (valueP == NULL)
  {
    languageMapP->type     = KjString;
    languageMapP->value.s  = (char*) "empty languageMap";
  }
  else if (valueP->type == KjString)
  {
    languageMapP->type     = KjString;
    languageMapP->value.s  = valueP->value.s;
  }
  else  // Array
  {
    languageMapP->type              = KjArray;
    languageMapP->value.firstChildP = valueP->value.firstChildP;
    languageMapP->lastChild         = valueP->lastChild;
    valueP->value.firstChildP       = NULL;
    valueP->lastChild               = NULL;
  }

  languageMapP->name = (char*) "value";
  typeP->value.s     = (char*) "Property";

  if (valueP != NULL)
  {
    KjNode* langP = kjString(orionldState.kjsonP, "lang", valueP->name);  // The name of the chosen language
    kjChildAdd(attrP, langP);
  }
}
