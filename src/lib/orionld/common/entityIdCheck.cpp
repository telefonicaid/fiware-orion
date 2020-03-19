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
}

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "orionld/common/orionldErrorResponse.h"                 // OrionldResponseErrorType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/entityErrorPush.h"                      // entityErrorPush
#include "orionld/common/urlCheck.h"                             // urlCheck
#include "orionld/common/urnCheck.h"                             // urnCheck

// -----------------------------------------------------------------------------
//
// entityIdCheck -
//
bool entityIdCheck(KjNode* entityIdNodeP, bool duplicatedId, KjNode* errorsArrayP)
{
  // Entity ID is mandatory
  if (entityIdNodeP == NULL)
  {
    LM_W(("Bad Input (CREATE/UPSERT: mandatory field missing: entity::id)"));
    entityErrorPush(errorsArrayP, "no entity::id", OrionldBadRequestData, "mandatory field missing", "entity::id", 400);
    return false;
  }

  // Entity ID must be a string
  if (entityIdNodeP->type != KjString)
  {
    LM_W(("Bad Input (CREATE/UPSERT: entity::id not a string)"));
    entityErrorPush(errorsArrayP, "invalid entity::id", OrionldBadRequestData, "field with invalid type", "entity::id", 400);
    return false;
  }

  // Entity ID must be a valid URI
  char* detail;
  if (!urlCheck(entityIdNodeP->value.s, &detail) && !urnCheck(entityIdNodeP->value.s, &detail))
  {
    LM_W(("Bad Input (CREATE/UPSERT: entity::id is a string but not a valid URI)"));
    entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "Not a URI", entityIdNodeP->value.s, 400);
    return false;
  }

  // Entity ID must not be duplicated
  if (duplicatedId == true)
  {
    LM_W(("Bad Input (CREATE/UPSERT: Duplicated entity::id)"));
    entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "Duplicated field", "entity::id", 400);
    return false;
  }

  return true;
}
