/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildAdd
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"

#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl, userAgentHeader
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/notifications/previousValueAdd.h"              // Own interface



// -----------------------------------------------------------------------------
//
// previousValueAdd -
//
void previousValueAdd(KjNode* attrP, const char* attrLongName)
{
  KjNode* typeP     = kjLookup(attrP, "type");
  char*   fieldName = (char*) "previousValue";
  KjNode* previousP = NULL;

  if (typeP != NULL)
  {
    if (strcmp(typeP->value.s, "Relationship") == 0)
      fieldName = (char*) "previousObject";
    else if (strcmp(typeP->value.s, "GeoProperty") == 0)
      fieldName = (char*) "previousValue";
    else if (strcmp(typeP->value.s, "LanguageProperty") == 0)
      fieldName = (char*) "previousLanguageMap";
  }

  if (orionldState.previousValues != NULL)
  {
    previousP = kjLookup(orionldState.previousValues, attrLongName);
    if (previousP == NULL)
    {
      // Might be that eqForDot is needed ...
      char* eqName = kaStrdup(&orionldState.kalloc, attrLongName);
      dotForEq(eqName);
      previousP = kjLookup(orionldState.previousValues, eqName);
    }

    if (previousP != NULL)
    {
      previousP = kjClone(orionldState.kjsonP, previousP);
      previousP->name = fieldName;
    }
  }
  // else if (orionldState.previousValueArray != NULL)  // When more than one entity

  if (previousP != NULL)
  {
    LM_T(LmtShowChanges, ("Adding '%s' to attribute '%s'", previousP->name, attrP->name));
    kjChildAdd(attrP, previousP);
  }
}
