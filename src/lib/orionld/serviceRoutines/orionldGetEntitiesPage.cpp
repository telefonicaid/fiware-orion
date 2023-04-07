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
#include "kjson/kjLookup.h"                                         // kjLookup
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // LmtMongoc

#include "orionld/common/orionldState.h"                            // orionldState, orionldEntityMapCount
#include "orionld/common/orionldError.h"                            // orionldError
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
  {
    LM_T(LmtEntityMap, ("offset (%d) >= orionldEntityMapCount (%d)", offset, orionldEntityMapCount));
    return true;
  }

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

  //
  // What we have here is a number of "slots" in the entity map, each slot with the layout:
  //
  // "urn:cp3:entities:E30" [ "urn:Reg1", "urn:Reg2", null ]
  //
  // To avoid making a DB query, of forwarded request, for each and every entity in the slots,
  // we must here group them into:
  //
  //   {
  //     "urn:reg1": [ "urn:E1", ... "urn:En" ],
  //     "urn:reg2": [ "urn:E1", ... "urn:En" ],
  //     "localDB":  [ "urn:E1", ... "urn:En" ]
  //   }
  // ]
  //
  // Once that array is ready, we can send forwarded requests or query the local DB
  //
  KjNode* sources = kjObject(orionldState.kjsonP, NULL);

  for (int ix = 0; ix < limit; ix++)
  {
    if (entityMap == NULL)
      break;

    char* entityId = entityMap->name;

    for (KjNode* regP = entityMap->value.firstChildP; regP != NULL; regP = regP->next)
    {
      const char* regId    = (regP->type == KjString)? regP->value.s : "localDB";
      KjNode*     regArray = kjLookup(sources, regId);

      if (regArray == NULL)
      {
        regArray = kjArray(orionldState.kjsonP, regId);
        kjChildAdd(sources, regArray);
      }

      KjNode* entityIdNodeP = kjString(orionldState.kjsonP, NULL, entityId);
      kjChildAdd(regArray, entityIdNodeP);
    }

    entityMap = entityMap->next;
  }

  kjTreeLog(sources, "sources", LmtSR);

  for (KjNode* sourceP = sources->value.firstChildP; sourceP != NULL; sourceP = sourceP->next)
  {
    LM_T(LmtSR, ("Query '%s' for:", sourceP->name));
    for (KjNode* entityNodeP = sourceP->value.firstChildP; entityNodeP != NULL; entityNodeP = entityNodeP->next)
    {
      LM_T(LmtSR, ("  o %s", entityNodeP->value.s));
    }
  }

  return true;
}
