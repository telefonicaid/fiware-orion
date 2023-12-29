/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <string.h>                                            // strlen
#include <string>                                              // std::string
#include <vector>                                              // std::vector

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjChildAdd, kjObject, kjArray, ...
#include "kalloc/kaStrdup.h"                                   // kaStrdup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/kjTree/kjEntityNormalizedToConcise.h"        // kjEntityNormalizedToConcise
#include "orionld/kjTree/kjEntityNormalizedToSimplified.h"     // kjEntityNormalizedToSimplified
#include "orionld/payloadCheck/pcheckQuery.h"                  // pcheckQuery
#include "orionld/mongoCppLegacy/mongoCppLegacyEntitiesQuery.h"  // mongoCppLegacyEntitiesQuery
#include "orionld/dbModel/dbModelToApiEntity.h"                // dbModelToApiEntity2
#include "orionld/legacyDriver/legacyPostQuery.h"              // Own Interface



// ----------------------------------------------------------------------------
//
// legacyPostQuery -
//
// POST /ngsi-ld/v1/entityOperations/query
//
bool legacyPostQuery(void)
{
  if (orionldState.requestTree->type != KjObject)
  {
    orionldError(OrionldBadRequestData, "Bad Request", "The payload data for POST Query must be a JSON Object", 400);
    return false;
  }

  KjNode*  entitiesP = NULL;
  KjNode*  attrsP    = NULL;
  QNode*   qTree     = NULL;
  KjNode*  geoqP     = NULL;
  char*    lang      = NULL;

  // pcheckQuery makes sure the Payload Data is correct and it expands all fields that should be expanded
  if (pcheckQuery(orionldState.requestTree, &entitiesP, &attrsP, &qTree, &geoqP, &lang) == false)
    return false;

  int      count  = 0;
  int      limit  = orionldState.uriParams.limit;
  int      offset = orionldState.uriParams.offset;
  int*     countP = (orionldState.uriParams.count == true)? &count : NULL;
  KjNode*  dbEntityArray;

  if ((dbEntityArray = mongoCppLegacyEntitiesQuery(entitiesP, attrsP, qTree, geoqP, limit, offset, countP)) == NULL)
  {
    // Not an error - just "nothing found" - return an empty array
    orionldState.responsePayload = (char*) "[]";
    if (countP != NULL)
      orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, 0);

    return true;
  }

  //
  // Now the "raw db entities" must be fixed.
  // Also, do we need another field in the payload for filtering out which attrs to be returned???
  //
  orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);

  if (dbEntityArray->value.firstChildP != NULL)
  {
    for (KjNode* dbEntityP = dbEntityArray->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
    {
      KjNode*                entityP;
      OrionldProblemDetails  pd;

      if ((entityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, orionldState.out.format, lang, true, &pd)) == NULL)
      {
        LM_E(("Database Error (%s: %s)", pd.title, pd.detail));
        orionldState.httpStatusCode = 500;
        return false;
      }

      kjChildAdd(orionldState.responseTree, entityP);
    }
  }
  else
    orionldState.noLinkHeader = true;

  orionldState.httpStatusCode = 200;

  if (countP != NULL)
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);

  return true;
}
