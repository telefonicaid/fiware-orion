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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjArray, ...
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/types/StringArray.h"                         // StringArray
#include "orionld/payloadCheck/pCheckUri.h"                    // pCheckUri
#include "orionld/common/batchEntityStringArrayPopulate.h"     // Own interface



// ----------------------------------------------------------------------------
//
// batchEntityStringArrayPopulate -
//
int batchEntityStringArrayPopulate(KjNode* requestTree, StringArray* eIdArrayP, KjNode* errorsArrayP, bool entityTypeMandatory)
{
  int     idIndex = 0;
  KjNode* eP      = requestTree->value.firstChildP;
  KjNode* next;

  while (eP != NULL)
  {
    next = eP->next;

    KjNode* idNodeP = kjLookup(eP, "id");

    if (idNodeP == NULL)
    {
      idNodeP = kjLookup(eP, "@id");

      if (idNodeP == NULL)
      {
        entityErrorPush(errorsArrayP, "No Entity::id", OrionldBadRequestData, "Mandatory field missing", "Entity::id", 400);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }

      idNodeP->name = (char*) "id";  // From this point, there are no @id, only id
    }

    if (idNodeP->type != KjString)
    {
      entityErrorPush(errorsArrayP, "Invalid Entity::id", OrionldBadRequestData, "Invalid JSON type", kjValueType(idNodeP->type), 400);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }

    if (pCheckUri(idNodeP->value.s, NULL, true) == false)
    {
      entityErrorPush(errorsArrayP, idNodeP->value.s, OrionldBadRequestData, "Invalid URI", "Entity::id", 400);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }


    //
    // Entity Type check, only if the type is mandatory
    //
    if (entityTypeMandatory == true)
    {
      KjNode* typeNodeP = kjLookup(eP, "type");

      if (typeNodeP == NULL)
      {
        typeNodeP = kjLookup(eP, "@type");

        if (typeNodeP == NULL)
        {
          entityErrorPush(errorsArrayP, "No Entity::type", OrionldBadRequestData, "Mandatory field missing", "Entity::type", 400);
          kjChildRemove(orionldState.requestTree, eP);
          eP = next;
          continue;
        }

        typeNodeP->name = (char*) "type";  // From this point, there are no @type, only type
      }

      if (typeNodeP->type != KjString)
      {
        entityErrorPush(errorsArrayP, "Invalid Entity::type", OrionldBadRequestData, "Invalid JSON type", kjValueType(typeNodeP->type), 400);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }
    }

    eIdArrayP->array[idIndex++] = idNodeP->value.s;
    eP = next;
  }

  eIdArrayP->items = idIndex;
  return eIdArrayP->items;
}
