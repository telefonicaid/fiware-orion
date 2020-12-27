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



// -----------------------------------------------------------------------------
//
// entityTypeCheck -
//
bool entityTypeCheck(KjNode* entityTypeNodeP, bool duplicatedType, char* entityId, bool typeMandatory, KjNode* errorsArrayP)
{
  // Entity TYPE is mandatory?
  if ((typeMandatory == true) && (entityTypeNodeP == NULL))
  {
    LM_W(("Bad Input (UPSERT: mandatory field missing: entity::type)"));
    entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "mandatory field missing", "entity::type", 400, false);
    return false;
  }

  // Entity TYPE must not be duplicated
  if (duplicatedType == true)
  {
    LM_W(("KZ: Bad Input (UPSERT: Duplicated entity::type)"));
    entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Duplicated field", "entity::type", 400, false);
    return false;
  }

  // Entity TYPE must be a string
  if ((entityTypeNodeP != NULL) && (entityTypeNodeP->type != KjString))
  {
    LM_W(("Bad Input (UPSERT: entity::type not a string)"));
    entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "field with invalid type", "entity::type", 400, false);
    return false;
  }

  return true;
}
