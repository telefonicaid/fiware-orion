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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "orionld/http/httpHeaderLocationAdd.h"                  // httpHeaderLocationAdd
#include "orionld/http/httpHeaderLinkAdd.h"                      // httpHeaderLinkAdd
#include "orionld/types/DistOpType.h"                            // DistOpType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjChildCount.h"                         // kjChildCount
#include "orionld/kjTree/kjSort.h"                               // kjStringArraySort
#include "orionld/common/responseFix.h"                          // Own interface


// -----------------------------------------------------------------------------
//
// responseFix -
//
void responseFix(KjNode* responseBody, DistOpType operation, int okCode, const char* entityId)
{
  KjNode* successArray = kjLookup(responseBody, "success");
  KjNode* failureArray = kjLookup(responseBody, "failure");
  int     failures     = (failureArray == NULL)? 0 : kjChildCount(failureArray);
  int     successes    = (successArray == NULL)? 0 : kjChildCount(successArray);

  if (failureArray != NULL)  failureArray->name = (char*) "notUpdated";
  if (successArray != NULL)  successArray->name = (char*) "updated";

  if (failures == 0)
  {
    orionldState.httpStatusCode = okCode;
    orionldState.responseTree   = NULL;

    if ((okCode == 201) && (entityId != NULL))
      httpHeaderLocationAdd("/ngsi-ld/v1/entities/", entityId, orionldState.tenantP->tenant);
  }
  else if ((successes == 0) && (failures == 1))
  {
    KjNode* errorP      = failureArray->value.firstChildP;
    KjNode* statusCodeP = kjLookup(errorP, "statusCode");
    int     statusCode  = (statusCodeP != NULL)? statusCodeP->value.i : 400;

    if (statusCodeP != NULL)
      kjChildRemove(errorP, statusCodeP);

    if (operation == DoUpdateAttrs)
    {
      KjNode* attributesP = kjLookup(errorP, "attributes");
      if (attributesP != NULL)
        kjChildRemove(errorP, attributesP);
    }

    orionldState.httpStatusCode = statusCode;
    orionldState.responseTree   = errorP;

    httpHeaderLinkAdd(orionldState.contextP->url);  // For attribute names to be expanded by receiver
  }
  else
  {
    if (successes > 1)
      kjStringArraySort(successArray);

    orionldState.httpStatusCode = 207;
    orionldState.responseTree   = responseBody;

    httpHeaderLinkAdd(orionldState.contextP->url);  // For attribute names to be expanded by receiver

    if ((okCode == 201) && (entityId != NULL) && (successes >= 1))
      httpHeaderLocationAdd("/ngsi-ld/v1/entities/", entityId, orionldState.tenantP->tenant);
  }
}
