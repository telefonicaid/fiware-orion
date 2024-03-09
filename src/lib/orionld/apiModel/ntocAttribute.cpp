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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // kjTreeLog
#include "orionld/kjTree/kjChildCount.h"                         // kjChildCount
#include "orionld/kjTree/kjSysAttrsRemove.h"                     // kjSysAttrsRemove
#include "orionld/apiModel/langFixNormalized.h"                  // langFixNormalized
#include "orionld/apiModel/ntocSubAttribute.h"                   // ntocSubAttribute
#include "orionld/apiModel/ntocAttribute.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// ntocAttribute -
//
void ntocAttribute(KjNode* attrP, const char* lang, bool sysAttrs)
{
  //
  // If ARRAY, we're dealing with datasetId ...  - later!
  //
  if (attrP->type == KjArray)
  {
    LM_E(("Multi-Attribute (datasetId) not supported, sorry ..."));
    return;
  }

  kjTreeLog(attrP, "attribute to convert from Normalized to Concise", LmtFormat);

  // 1. Remove the sysAttrs, if so requested
  if (sysAttrs == false)
    kjSysAttrsRemove(attrP, 0);


  // 2. LanguageProperty?
  KjNode* typeP = kjLookup(attrP, "type");
  if ((lang != NULL) && (typeP != NULL) && (strcmp(typeP->value.s, "LanguageProperty") == 0))
  {
    KjNode* languageMapP = kjLookup(attrP, "languageMap");
    langFixNormalized(attrP, typeP, languageMapP, lang);  // the concise part comes later
  }

  // 3. Remove the attribute type
  if (typeP != NULL)
    kjChildRemove(attrP, typeP);

  // 4. If "value" is the only child left - Simplified
  KjNode* valueP     = kjLookup(attrP, "value");
  int     attrFields = kjChildCount(attrP);

  if ((valueP != NULL) && (attrFields == 1))  // Simplified
  {
    attrP->value     = valueP->value;
    attrP->type      = valueP->type;
    attrP->lastChild = valueP->lastChild;

    return;
  }

  // Loop over sub-attributes
  for (KjNode* fieldP = attrP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next)
  {
    if (fieldP->name == NULL)
      continue;

    if (strcmp(fieldP->name, "type")        == 0)  continue;
    if (strcmp(fieldP->name, "value")       == 0)  continue;
    if (strcmp(fieldP->name, "object")      == 0)  continue;
    if (strcmp(fieldP->name, "languageMap") == 0)  continue;
    if (strcmp(fieldP->name, "json")        == 0)  continue;
    if (strcmp(fieldP->name, "vocab")       == 0)  continue;
    if (strcmp(fieldP->name, "createdAt")   == 0)  continue;
    if (strcmp(fieldP->name, "modifiedAt")  == 0)  continue;
    if (strcmp(fieldP->name, "observedAt")  == 0)  continue;
    if (strcmp(fieldP->name, "unitCode")    == 0)  continue;
    if (strcmp(fieldP->name, "datasetId")   == 0)  continue;
    if (strcmp(fieldP->name, "lang")        == 0)  continue;

    ntocSubAttribute(fieldP, lang, sysAttrs);
  }
}
