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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjArray, kjString, kjChildAdd, ...
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/forwarding/DistOp.h"                           // DistOp
#include "orionld/forwarding/distOpSuccess.h"                    // Own interface



extern void updatedAttr404Purge(KjNode* failureV, char* attrName);
// -----------------------------------------------------------------------------
//
// distOpSuccess -
//
void distOpSuccess(KjNode* responseBody, DistOp* distOpP)
{
  KjNode* successV = kjLookup(responseBody, "success");
  KjNode* failureV = kjLookup(responseBody, "failure");

  if (successV == NULL)
  {
    successV = kjArray(orionldState.kjsonP, "success");
    kjChildAdd(responseBody, successV);
  }

  for (KjNode* attrNameP = distOpP->requestBody->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
  {
    if (strcmp(attrNameP->name, "id") == 0)
      continue;
    if (strcmp(attrNameP->name, "type") == 0)
      continue;

    char* attrLongName = kaStrdup(&orionldState.kalloc, attrNameP->name);
    eqForDot(attrLongName);
    char* alias = orionldContextItemAliasLookup(orionldState.contextP, attrLongName, NULL, NULL);

    if (kjStringValueLookupInArray(successV, alias) == NULL)
    {
      KjNode* aNameP = kjString(orionldState.kjsonP, NULL, alias);
      LM_T(LmtDistOp207, ("Adding '%s' to successV", alias));
      kjChildAdd(successV, aNameP);

      updatedAttr404Purge(failureV, alias);
    }
    else
      LM_T(LmtDistOp207, ("NOT adding attribute '%s' to successV (it's already present)", alias));
  }
}
