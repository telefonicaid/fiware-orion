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

#include "orionld/kjTree/kjChildCount.h"                            // kjChildCount
#include "orionld/kjTree/kjEntityNormalizedToConcise.h"             // Own interface



// ----------------------------------------------------------------------------
//
// kjEntityNormalizedToConcise -
//
void kjEntityNormalizedToConcise(KjNode* outputP)
{
  for (KjNode* attrP = outputP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type != KjObject)
      continue;

    KjNode* typeP  = kjLookup(attrP, "type");
    KjNode* valueP = kjLookup(attrP, "value");

    if ((typeP != NULL) && (valueP != NULL) && (strcmp(typeP->value.s, "Property") == 0))
      kjChildRemove(attrP, typeP);

    // Same for GeoProperty
    if ((typeP != NULL) && (valueP) && (strcmp(typeP->value.s, "GeoProperty") == 0))
      kjChildRemove(attrP, typeP);

    // If only value - make it keyValues
    if ((valueP != NULL) && (kjChildCount(attrP) == 1))
    {
      attrP->type      = valueP->type;
      attrP->value     = valueP->value;
      continue;
    }

    // If Relationship, remove typeP - leave "object"
    if ((typeP != NULL) && (strcmp(typeP->value.s, "Relationship") == 0))
      kjChildRemove(attrP, typeP);

    // If LanguageProperty, remove typeP - leave "languageMap"
    if ((typeP != NULL) && (strcmp(typeP->value.s, "LanguageProperty") == 0))
      kjChildRemove(attrP, typeP);

    // Same same for Sub-Attributes
    kjEntityNormalizedToConcise(attrP);
  }
}
