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
#include <string>                                              // std::string

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderLocationAdd
#include "rest/OrionError.h"                                   // OrionError
#include "apiTypesV2/Registration.h"                           // Registration
#include "mongoBackend/mongoRegistrationGet.h"                 // mongoRegistrationGet
#include "mongoBackend/mongoRegistrationCreate.h"              // mongoRegistrationCreate

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/kjTree/kjTreeToRegistration.h"               // kjTreeToRegistration
#include "orionld/context/orionldCoreContext.h"                // ORIONLD_CORE_CONTEXT_URL
#include "orionld/mongoBackend/mongoLdRegistrationGet.h"       // mongoLdRegistrationGet
#include "orionld/serviceRoutines/orionldPostRegistrations.h"  // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPostRegistrations -
//
bool orionldPostRegistrations(ConnectionInfo* ciP)
{
  ngsiv2::Registration  reg;
  std::string           regId;
  OrionError            oError;
  char*                 regIdP = NULL;

  //
  // Registration ID given by user? - if so, we need to check for Registration already exists
  //
  if (orionldState.payloadIdNode != NULL)
  {
    char*   regId = orionldState.payloadIdNode->value.s;
    int     statusCode;
    char*   details;

    // mongoLdRegistrationGet takes the req semaphore
    if (mongoLdRegistrationGet(NULL, regId, orionldState.tenant, &statusCode, &details) == true)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Registration already exists", regId);
      return false;
    }
  }

  //
  // Fix context
  //
  if (orionldState.contextP != NULL)
    reg.ldContext = orionldState.contextP->url;
  else
    reg.ldContext = ORIONLD_CORE_CONTEXT_URL;

  //
  // Translate the incoming KjNode tree into a ngsiv2::Registration
  //
  if (kjTreeToRegistration(&reg, &regIdP) == false)
  {
    LM_E(("kjTreeToRegistration FAILED"));
    // orionldErrorResponseCreate is invoked by kjTreeToRegistration
    return false;
  }


  //
  // Create the Registration
  //
  mongoRegistrationCreate(&reg,
                          orionldState.tenant,
                          ciP->servicePathV[0],
                          &regId,
                          &oError);

  // FIXME: Check oError for failure!
  orionldState.httpStatusCode = SccCreated;

  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/csourceRegistrations/", regIdP);

  return true;
}
