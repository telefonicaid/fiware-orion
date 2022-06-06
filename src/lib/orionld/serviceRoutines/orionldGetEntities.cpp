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
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd, HttpResultsCount
#include "orionld/legacyDriver/legacyGetEntities.h"              // legacyGetEntities
#include "orionld/mongoc/mongocEntitiesQuery.h"                  // mongocEntitiesQuery
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/q/qLex.h"                                      // qLex
#include "orionld/q/qParse.h"                                    // qParse
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity2
#include "orionld/serviceRoutines/orionldGetEntities.h"          // Own interface



// ----------------------------------------------------------------------------
//
// orionldGetEntities -
//
bool orionldGetEntities(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyGetEntities();

  QNode* qNode = NULL;
  if (orionldState.uriParams.q != NULL)
  {
    QNode* qList;
    char*  title;
    char*  detail;

    qList = qLex(orionldState.uriParams.q, true, &title, &detail);
    if (qList == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Q-Filter", detail, 400);
      LM_RE(false, ("Error (qLex: %s: %s)", title, detail));
    }

    qNode = qParse(qList, NULL, true, true, &title, &detail);  // 3rd parameter: forDb=true
    if (qNode == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Q-Filter", detail, 400);
      LM_RE(NULL, ("Error (qParse: %s: %s) - but, the subscription will be inserted in the sub-cache without 'q'", title, detail));
    }
  }

  //
  // NOTE:
  //   What if an entity id list is given, but with one single entity id?
  //   In general these two requests should be equivalent:
  //     - GET /entities?id=urn:ngsi-ld:T:E1
  //     - GET /entities/urn:ngsi-ld:T:E1
  //
  //   However, if more URI params are given, the similarities aren't that big anymore.
  //   E.g., if attrs is given:
  //     - GET /entities?id=urn:ngsi-ld:T:E1&attrs=P1  [1]
  //     - GET /entities/urn:ngsi-ld:T:E1&attrs=P1     [2]
  //
  //   Imagine that the entity urn:ngsi-ld:T:E1 doesn't have an attribute P1.
  //     - The query (1) would give back an empty arry - no match
  //     - The retrieval (2) would give back the entity withoput any attributes
  //
  //   So, perhaps we should play it safe way and NOT EVER "redirect" to GET /entities/{entityId}
  //

  // According to the spec, id takes precedence over idPattern, so, if both are present, idPattern is NULLed out
  char* idPattern = orionldState.uriParams.idPattern;
  if ((orionldState.in.idList.items > 0) && (idPattern != NULL))
    idPattern = NULL;

  int64_t       count;
  KjNode*       dbEntityArray   = mongocEntitiesQuery(&orionldState.in.typeList, &orionldState.in.idList, idPattern, &orionldState.in.attrList, qNode, &count);
  KjNode*       apiEntityArray  = kjArray(orionldState.kjsonP, NULL);
  RenderFormat  rf              = RF_NORMALIZED;

  if      (orionldState.uriParamOptions.concise   == true) rf = RF_CONCISE;
  else if (orionldState.uriParamOptions.keyValues == true) rf = RF_KEYVALUES;

  for (KjNode* dbEntityP = dbEntityArray->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
  {
    KjNode* apiEntityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, rf, orionldState.uriParams.lang, &orionldState.pd);
    kjChildAdd(apiEntityArray, apiEntityP);
  }
  orionldState.responseTree = apiEntityArray;

  if (orionldState.uriParams.count == true)
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);

  // If empty result array, no Link header is needed
  if (orionldState.responseTree->value.firstChildP == NULL)
    orionldState. noLinkHeader = true;

  return true;
}
