/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
* Author: Ken Zangelin and Gabriel Quaresma
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_URI
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocEntityDelete.h"                   // mongocEntityDelete
#include "orionld/serviceRoutines/orionldDeleteEntity.h"         // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteEntity -
//
bool orionldDeleteEntity(void)
{
  char* entityId   = orionldState.wildcard[0];
  char* entityType = orionldState.uriParams.type;

  // Make sure the Entity ID is a valid URI
  PCHECK_URI(entityId, true, 0, "Invalid Entity ID", "Must be a valid URI", 400);

  if (mongocEntityLookup(entityId, entityType, NULL, NULL) == NULL)  // FIXME: Overkill to extract the entire entity from the DB
  {
    orionldError(OrionldResourceNotFound, "Entity not found", entityId, 404);
    return false;
  }

  if (mongocEntityDelete(entityId) == false)
  {
    orionldError(OrionldInternalError, "Database Error", "mongocEntityDelete failed", 500);
    return false;
  }

  orionldState.httpStatusCode = SccNoContent;  // 204

  return true;
}
