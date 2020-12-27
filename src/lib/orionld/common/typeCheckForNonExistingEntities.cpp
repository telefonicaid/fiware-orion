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
* Author: Gabriel Quaresma and Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "orionld/common/orionldErrorResponse.h"                 // OrionldResponseErrorType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/entityErrorPush.h"                      // entityErrorPush
#include "orionld/common/entityLookupById.h"                     // entityLookupById
#include "orionld/common/removeArrayEntityLookup.h"              // removeArrayEntityLookup


// ----------------------------------------------------------------------------
//
// typeCheckForNonExistingEntities -
//
bool typeCheckForNonExistingEntities(KjNode* incomingTree, KjNode* idTypeAndCreDateFromDb, KjNode* errorsArrayP, KjNode* removeArray)
{
  KjNode* inNodeP = incomingTree->value.firstChildP;
  KjNode* next;

  while (inNodeP != NULL)
  {
    //
    // entities that weren't found in the DB MUST contain entity::type
    //
    KjNode* inEntityIdNodeP = kjLookup(inNodeP, "id");

    if (inEntityIdNodeP == NULL)
      inEntityIdNodeP = kjLookup(inNodeP, "@id");

    if (inEntityIdNodeP == NULL)  // Entity ID is mandatory
    {
      LM_E(("KZ: Invalid Entity: Mandatory field entity::id is missing"));
      entityErrorPush(errorsArrayP, "No ID", OrionldBadRequestData, "Invalid Entity", "Mandatory field entity::id is missing", 400, true);
      next = inNodeP->next;
      kjChildRemove(incomingTree, inNodeP);
      inNodeP = next;
      continue;
    }

    KjNode* dbEntityId = NULL;

    // Lookup the entity::id in what came from the database - if anything
    if (idTypeAndCreDateFromDb != NULL)
      dbEntityId = entityLookupById(idTypeAndCreDateFromDb, inEntityIdNodeP->value.s);

    if (dbEntityId == NULL)  // This Entity is to be created - "type" is mandatory!
    {
      KjNode* inEntityTypeNodeP = kjLookup(inNodeP, "type");

      if (inEntityTypeNodeP == NULL)
        inEntityTypeNodeP = kjLookup(inNodeP, "@type");

      if (inEntityTypeNodeP == NULL)
      {
        LM_E(("KZ: Invalid Entity: Mandatory field entity::type is missing"));
        entityErrorPush(errorsArrayP, inEntityIdNodeP->value.s, OrionldBadRequestData, "Invalid Entity", "Mandatory field entity::type is missing", 400, false);

        if (removeArray != NULL)
        {
          KjNode* entityInRemoveArray = removeArrayEntityLookup(removeArray, inEntityIdNodeP->value.s);

          if (entityInRemoveArray != NULL)
            kjChildRemove(removeArray, entityInRemoveArray);
        }

        next = inNodeP->next;
        kjChildRemove(incomingTree, inNodeP);
        inNodeP = next;
        continue;
      }
    }

    inNodeP = inNodeP->next;
  }

  return true;
}
