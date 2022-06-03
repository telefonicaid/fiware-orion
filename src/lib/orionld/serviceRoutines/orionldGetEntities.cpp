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

  int64_t      count;
  KjNode*      dbEntityArray   = mongocEntitiesQuery(&orionldState.in.typeList, &count);
  KjNode*      apiEntityArray  = kjArray(orionldState.kjsonP, NULL);
  RenderFormat rf              = RF_NORMALIZED;

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

  return true;
}
