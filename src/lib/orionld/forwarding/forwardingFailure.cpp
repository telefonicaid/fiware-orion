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
#include "kjson/kjBuilder.h"                                     // kjArray, kljString, kjChildAdd, ...
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/types/OrionldResponseErrorType.h"              // OrionldResponseErrorType
#include "orionld/forwarding/forwardingFailure.h"                // Own interface



// -----------------------------------------------------------------------------
//
// forwardingFailure -
//
void forwardingFailure(KjNode* responseBody, ForwardPending* fwdPendingP, OrionldResponseErrorType errorCode, const char* title, const char* detail, int httpStatus)
{
  KjNode* failureV = kjLookup(responseBody, "failure");

  if (failureV == NULL)
  {
    failureV = kjArray(orionldState.kjsonP, "failure");
    kjChildAdd(responseBody, failureV);
  }

  KjNode* regIdP      = (fwdPendingP != NULL)? kjLookup(fwdPendingP->regP->regTree, "id") : NULL;
  KjNode* regIdNodeP  = (regIdP != NULL)? kjString(orionldState.kjsonP, "registrationId", regIdP->value.s) : NULL;
  KjNode* error       = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrV       = kjArray(orionldState.kjsonP, "attributes");
  KjNode* statusCodeP = kjInteger(orionldState.kjsonP, "statusCode", httpStatus);
  KjNode* titleP      = kjString(orionldState.kjsonP, "title", title);
  KjNode* detailP     = kjString(orionldState.kjsonP, "detail", detail);

  if (regIdNodeP != NULL)
    kjChildAdd(error, regIdNodeP);

  kjChildAdd(error, attrV);
  kjChildAdd(error, statusCodeP);
  kjChildAdd(error, titleP);
  kjChildAdd(error, detailP);

  for (KjNode* attrNameP = fwdPendingP->body->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
  {
    if (strcmp(attrNameP->name, "id") == 0)
      continue;
    if (strcmp(attrNameP->name, "type") == 0)
      continue;

    char*   alias  = orionldContextItemAliasLookup(orionldState.contextP, attrNameP->name, NULL, NULL);
    KjNode* aNameP = kjString(orionldState.kjsonP, NULL, alias);
    kjChildAdd(attrV, aNameP);
  }

  kjChildAdd(failureV, error);
}



