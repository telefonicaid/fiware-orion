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

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/distOp/distOpSuccess.h"                        // Own interface



extern void updatedAttr404Purge(KjNode* failureV, char* attrName);
// -----------------------------------------------------------------------------
//
// attrNameToSuccess -
//
static void attrNameToSuccess(KjNode* successV, KjNode* failureV, char* attrName)
{
  char* attrLongName = kaStrdup(&orionldState.kalloc, attrName);
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



// -----------------------------------------------------------------------------
//
// distOpSuccess -
//
void distOpSuccess(KjNode* responseBody, DistOp* distOpP, const char* entityId, char* attrName)
{
  KjNode* successV = kjLookup(responseBody, "success");
  KjNode* failureV = kjLookup(responseBody, "failure");

  if (successV == NULL)
  {
    successV = kjArray(orionldState.kjsonP, "success");
    kjChildAdd(responseBody, successV);
  }

  if ((distOpP == NULL) && (attrName == NULL))
  {
    KjNode* entityIdNodeP = kjString(orionldState.kjsonP, NULL, entityId);
    kjChildAdd(successV, entityIdNodeP);
  }

  if (attrName != NULL)
    attrNameToSuccess(successV, failureV, attrName);
  else if (distOpP != NULL)
  {
    if (distOpP->attrName != NULL)
      attrNameToSuccess(successV, failureV, distOpP->attrName);
    else if (distOpP->requestBody != NULL)
    {
      for (KjNode* attrNameP = distOpP->requestBody->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
      {
        if (strcmp(attrNameP->name, "id") == 0)
          continue;
        if (strcmp(attrNameP->name, "type") == 0)
          continue;

        attrNameToSuccess(successV, failureV, attrNameP->name);
      }
    }
  }
}
