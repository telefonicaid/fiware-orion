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
#include "kjson/kjLookup.h"                                         // kjLookup
#include "kjson/kjBuilder.h"                                        // kjObject
}

#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/context/orionldContextItemAliasLookup.h"          // orionldContextItemAliasLookup
#include "orionld/dbModel/dbModelToEntityIdAndTypeObject.h"         // Own interface



// ----------------------------------------------------------------------------
//
// dbModelToEntityIdAndTypeObject - FIXME: rename to dbModelToEntityMap
//
// INPUT:  [ { "_id": { "id": "urn:E1", "type": "urn:...:T1" } }, { "_id": { "id": "urn:E2", "type": "urn:...:T2" } }, ... ]
// OUTPUT: ["urn:E1", "urn:E2", ...]
//
KjNode* dbModelToEntityIdAndTypeObject(KjNode* localDbMatches)
{
  KjNode* matchIds = kjArray(orionldState.kjsonP, NULL);

  for (KjNode* dbEntityP = localDbMatches->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
  {
    KjNode* _idP = kjLookup(dbEntityP, "_id");

    if (_idP == NULL)
      continue;   // DB Error !!!

    KjNode* idP   = kjLookup(_idP, "id");

    if (idP == NULL)
      continue;   // DB Error !!!

    KjNode* idNodeP = kjString(orionldState.kjsonP, NULL, idP->value.s);

    kjChildAdd(matchIds, idNodeP);
  }

  return matchIds;
}
