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
#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_URI
#include "orionld/mongoc/mongocRegistrationExists.h"             // mongocRegistrationExists
#include "orionld/mongoc/mongocRegistrationDelete.h"             // mongocRegistrationDelete
#include "orionld/legacyDriver/legacyDeleteRegistration.h"       // legacyDeleteRegistration
#include "orionld/regCache/regCacheItemRemove.h"                 // regCacheItemRemove
#include "orionld/serviceRoutines/orionldDeleteRegistration.h"   // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteRegistration -
//
bool orionldDeleteRegistration(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))
    return legacyDeleteRegistration();

  PCHECK_URI(orionldState.wildcard[0], true, 0, "Invalid Registration Identifier", orionldState.wildcard[0], 400);

  bool found = false;
  bool b     = mongocRegistrationExists(orionldState.wildcard[0], &found);
  if (b == false)
    return false;
  else if (found == false)
  {
    orionldError(OrionldResourceNotFound, "Registration not found", orionldState.wildcard[0], 404);
    return false;
  }

  if (regCacheItemRemove(orionldState.tenantP->regCache, orionldState.wildcard[0]) == false)
  {
    if (noCache == false)
      LM_W(("The registration '%s' does not exist in sub-cache ... (sub-cache is enabled)"));
  }

  if (mongocRegistrationDelete(orionldState.wildcard[0]) == false)
    return false;  // mongocRegistrationDelete calls orionldError, setting status code to 500 on error

  orionldState.httpStatusCode = 204;

  return true;
}
