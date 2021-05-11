/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/contextCache/orionldContextCacheDelete.h"      // orionldContextCacheDelete
#include "orionld/serviceRoutines/orionldDeleteContext.h"        // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteContext -
//
bool orionldDeleteContext(ConnectionInfo* ciP)
{
  if (strcmp(orionldState.wildcard[0], "Core") == 0)
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "Operation Not Allowed", "The core context cannot be deleted, sorry :)");
    orionldState.httpStatusCode = 403;
  }

  if (orionldContextCacheDelete(orionldState.wildcard[0]) == false)
  {
    orionldErrorResponseCreate(OrionldResourceNotFound, "Context Not Found", orionldState.wildcard[0]);
    orionldState.httpStatusCode = 404;
    return false;
  }

  orionldState.httpStatusCode = 204;
  return true;
}
