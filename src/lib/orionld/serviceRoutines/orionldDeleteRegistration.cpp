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
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/db/dbConfiguration.h"                          // dbRegistrationDelete
#include "orionld/payloadCheck/pcheckUri.h"                      // pcheckUri
#include "orionld/serviceRoutines/orionldDeleteRegistration.h"   // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteRegistration -
//
bool orionldDeleteRegistration(ConnectionInfo* ciP)
{
  char* detail;

  if (pcheckUri(orionldState.wildcard[0], &detail) == false)
  {
    LM_E(("uriCheck: %s", detail));
    orionldState.httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Context Source Registration Identifier", orionldState.wildcard[0]);  // FIXME: Include 'detail' and name (registrationId)
    return false;
  }

  if (dbRegistrationExists(orionldState.wildcard[0]) == false)
  {
    LM_E(("dbRegistrationExists says that the registration '%s' doesn't exist", orionldState.wildcard[0]));
    orionldState.httpStatusCode = 404;  // Not Found
    orionldErrorResponseCreate(OrionldResourceNotFound, "Context Source Registration not found", orionldState.wildcard[0]);
    return false;
  }

  if (dbRegistrationDelete(orionldState.wildcard[0]) == false)
  {
    LM_E(("dbRegistrationDelete failed - not found?"));
    orionldState.httpStatusCode = 404;  // Not Found
    orionldErrorResponseCreate(OrionldResourceNotFound, "Context Source Registration not found", orionldState.wildcard[0]);
    return false;
  }

  orionldState.httpStatusCode = SccNoContent;

  return true;
}
