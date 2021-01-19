/*
*
* Copyright 2020 FIWARE Foundation e.V.
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
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/entityIdAndTypeGet.h"                 // entityIdAndTypeGet
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/kjTree/kjEntityIdArrayExtract.h"             // Own interface



// ----------------------------------------------------------------------------
//
// entityIdPush - add ID to array
//
static KjNode* entityIdPush(KjNode* entityIdsArrayP, const char* entityId)
{
  KjNode* idNodeP = kjStringValueLookupInArray(entityIdsArrayP, entityId);

  if (idNodeP != NULL)  // The Entity with ID "entityId" is already present ...
    return NULL;

  idNodeP = kjString(orionldState.kjsonP, NULL, entityId);
  kjChildAdd(entityIdsArrayP, idNodeP);

  return idNodeP;
}



// -----------------------------------------------------------------------------
//
// kjEntityIdArrayExtract -
//
KjNode* kjEntityIdArrayExtract(KjNode* entityArray, KjNode* successArray, KjNode* errorArray)
{
  KjNode* entityP = entityArray->value.firstChildP;
  KjNode* next;
  KjNode* idArray = kjArray(orionldState.kjsonP, NULL);

  while (entityP)
  {
    next = entityP->next;

    char*   entityId;
    char*   entityType;

    // entityIdAndTypeGet calls entityIdCheck/entityTypeCheck that adds the entity in errorArray if needed
    if (entityIdAndTypeGet(entityP, &entityId, &entityType, errorArray) == false)
    {
      kjChildRemove(entityArray, entityP);
      entityP = next;
      continue;
    }

    //
    // Check Content-Type and @context in payload
    //
    KjNode* contextNodeP  = kjLookup(entityP, "@context");

    if ((orionldState.ngsildContent == true) && (contextNodeP == NULL))
    {
      LM_W(("Bad Input (Content-Type == application/ld+json, but no @context in payload data array item)"));
      entityErrorPush(errorArray,
                      entityId,
                      OrionldBadRequestData,
                      "Invalid payload",
                      "Content-Type is 'application/ld+json', but no @context in payload data array item",
                      400,
                      false);
      kjChildRemove(entityArray, entityP);
      entityP = next;
      continue;
    }

    if ((orionldState.ngsildContent == false) && (contextNodeP != NULL))
    {
      LM_W(("Bad Input (Content-Type is 'application/json', and an @context is present in the payload data array item)"));
      entityErrorPush(errorArray,
                      entityId,
                      OrionldBadRequestData,
                      "Invalid payload",
                      "Content-Type is 'application/json', and an @context is present in the payload data array item",
                      400,
                      false);
      kjChildRemove(entityArray, entityP);
      entityP = next;
      continue;
    }

    if ((contextNodeP != NULL) && (orionldState.linkHttpHeaderPresent == true))
    {
      LM_W(("Bad Input (@context present both in Link header and in payload data)"));
      entityErrorPush(errorArray,
                      entityId,
                      OrionldBadRequestData,
                      "Inconsistency between HTTP headers and payload data",
                      "@context present both in Link header and in payload data",
                      400,
                      false);
      kjChildRemove(entityArray, entityP);
      entityP = next;
      continue;
    }

    entityIdPush(idArray, entityId);

    entityP = next;
  }

  return idArray;
}
