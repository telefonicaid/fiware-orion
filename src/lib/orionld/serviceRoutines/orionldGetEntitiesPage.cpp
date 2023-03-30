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
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjRender.h"                                         // kjFastRender (for debugging purposes - LM_T)
#include "kjson/kjBuilder.h"                                        // kjArray, ...
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // LmtMongoc

#include "orionld/common/orionldState.h"                            // orionldState, orionldEntityMapCount
#include "orionld/serviceRoutines/orionldGetEntitiesPage.h"         // Own interface



// ----------------------------------------------------------------------------
//
// orionldGetEntitiesPage -
//
bool orionldGetEntitiesPage(KjNode* localDbMatches)
{
  int offset = orionldState.uriParams.offset;
  int limit  = orionldState.uriParams.limit;

  KjNode* entityArray = kjArray(orionldState.kjsonP, NULL);

  LM_T(LmtSR, ("entity map:          '%s'", orionldEntityMapId));
  LM_T(LmtSR, ("items in entity map:  %d",  orionldEntityMapCount));
  LM_T(LmtSR, ("offset:               %d",  offset));
  LM_T(LmtSR, ("limit:                %d",  limit));

  // HTTP Status code and payload body
  orionldState.responseTree   = entityArray;
  orionldState.httpStatusCode = 200;

  if (orionldState.uriParams.count == true)
  {
    LM_T(LmtEntityMap, ("%d entities match, in the entire federation", orionldEntityMapCount));
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, orionldEntityMapCount);
  }

  if (offset >= orionldEntityMapCount)
    return true;

  //
  // Fast forward to offset indes in the KJNode array
  //
  KjNode* entityMap = orionldEntityMap->value.firstChildP;
  for (int ix = 0; ix < offset; ix++)
  {
    entityMap = entityMap->next;
  }

  //
  // entityMap now points to the first entity to give back.
  // Must extract all parts of the entities, according to their array inside orionldEntityMap,
  // and merge them together (in case of distributed entities
  //
  for (int ix = 0; ix < limit; ix++)
  {
    if (entityMap == NULL)
      break;

    char buf[1024];
    kjFastRender(entityMap, buf);
    LM_T(LmtSR, ("Entity '%s': %s", entityMap->name, buf));
    entityMap = entityMap->next;
  }

  return true;
}
