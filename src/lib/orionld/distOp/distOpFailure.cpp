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

#include "logMsg/logMsg.h"                                       // LM_T

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/OrionldResponseErrorType.h"              // OrionldResponseErrorType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/distOp/distOpFailure.h"                        // Own interface



// -----------------------------------------------------------------------------
//
// distOpFailure -
//
void distOpFailure(KjNode* responseBody, DistOp* distOpP, const char* title, const char* detail, int httpStatus, const char* attrName)
{
  char* alias  = (attrName != NULL)? orionldContextItemAliasLookup(orionldState.contextP, attrName, NULL, NULL) : NULL;

  if (alias != NULL)
    LM_T(LmtDistOp207, ("Adding attribute '%s' to failureV", alias));
  else
    LM_T(LmtDistOp207, ("Adding item to failureV", alias));

  if (httpStatus == 404)
  {
    KjNode* successV = kjLookup(responseBody, "success");
    if ((successV != NULL) && (successV->value.firstChildP != NULL) && (alias != NULL))
    {
      if (kjStringValueLookupInArray(successV, alias) != NULL)
      {
        // The 404 attribute was updated elsewhere => not a 404
        LM_T(LmtDistOp207, ("NOT Adding attribute '%s' to failureV as it was found in successV", alias));
        return;
      }
    }
  }

  KjNode* failureV = kjLookup(responseBody, "failure");

  if (failureV == NULL)
  {
    failureV = kjArray(orionldState.kjsonP, "failure");
    kjChildAdd(responseBody, failureV);
  }

  KjNode* regIdP      = ((distOpP != NULL) && (distOpP->regP != NULL))? kjLookup(distOpP->regP->regTree, "id") : NULL;
  KjNode* regIdNodeP  = (regIdP  != NULL)? kjString(orionldState.kjsonP,  "registrationId", regIdP->value.s) : NULL;
  KjNode* error       = kjObject(orionldState.kjsonP,  NULL);
  KjNode* attrV       = kjArray(orionldState.kjsonP,   "attributes");
  KjNode* statusCodeP = kjInteger(orionldState.kjsonP, "statusCode", httpStatus);
  KjNode* titleP      = kjString(orionldState.kjsonP,  "title", title);
  KjNode* detailP     = (detail != NULL)? kjString(orionldState.kjsonP,  "detail", detail) : NULL;

  if (regIdNodeP != NULL)
    kjChildAdd(error, regIdNodeP);
  kjChildAdd(error, statusCodeP);
  kjChildAdd(error, titleP);
  if (detailP != NULL)
    kjChildAdd(error, detailP);

  if (attrName != NULL)  // local attribute, its name is given as parameter to this function
  {
    KjNode* aNameP = kjString(orionldState.kjsonP, NULL, alias);
    kjChildAdd(attrV, aNameP);
  }
  else if (distOpP != NULL)
  {
    if (distOpP->attrName != NULL)
    {
      char*   alias  = orionldContextItemAliasLookup(orionldState.contextP, distOpP->attrName, NULL, NULL);
      KjNode* aNameP = kjString(orionldState.kjsonP, NULL, alias);

      kjChildAdd(attrV, aNameP);
    }
    else if (distOpP->requestBody != NULL)
    {
      for (KjNode* attrNameP = distOpP->requestBody->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
      {
        if (strcmp(attrNameP->name, "id") == 0)
          continue;
        if (strcmp(attrNameP->name, "type") == 0)
          continue;

        char*   attrShortName  = orionldContextItemAliasLookup(orionldState.contextP, attrNameP->name, NULL, NULL);
        KjNode* aNameP         = kjString(orionldState.kjsonP, NULL, attrShortName);
        kjChildAdd(attrV, aNameP);
      }
    }
  }

  if (attrV->value.firstChildP != NULL)
    kjChildAdd(error, attrV);

  kjChildAdd(failureV, error);
}
