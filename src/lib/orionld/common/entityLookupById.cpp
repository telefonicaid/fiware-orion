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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/types/OrionldResponseErrorType.h"            // OrionldResponseErrorType



// -----------------------------------------------------------------------------
//
// entityLookupById - lookup an entity in an array of entities, by its entity-id
//
KjNode* entityLookupById(KjNode* entityArray, char* entityId)
{
  for (KjNode* entityP = entityArray->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* idNodeP = kjLookup(entityP, "id");

    if (idNodeP == NULL)
      idNodeP = kjLookup(entityP, "@id");

    if ((idNodeP != NULL) && (strcmp(idNodeP->value.s, entityId) == 0))  // If NULL, something is really wrong!!!
      return entityP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// entityLookupBy_id_Id - lookup an entity in an array of DB entities, by its entity-id
//
KjNode* entityLookupBy_id_Id(KjNode* entityArray, char* entityId, KjNode** entityTypeNodeP)
{
  for (KjNode* entityP = entityArray->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* _idNodeP = kjLookup(entityP, "_id");

    if (_idNodeP == NULL)
    {
      LM_W(("Database Error? (Entity without _id )"));
      continue;
    }

    KjNode* idNodeP = kjLookup(_idNodeP, "id");

    if (idNodeP == NULL)
      LM_W(("Database Error? (Entity _id without id)"));
    else if (idNodeP->type != KjString)
      LM_W(("Database Error? (Entity _id::id that is not a string)"));
    else if (strcmp(idNodeP->value.s, entityId) == 0)  // Match
    {
      if (entityTypeNodeP != NULL)
        *entityTypeNodeP = kjLookup(_idNodeP, "type");

      return entityP;
    }
  }

  return NULL;
}
