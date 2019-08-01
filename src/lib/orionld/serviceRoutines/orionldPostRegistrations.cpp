/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderLocationAdd
#include "rest/OrionError.h"                                   // OrionError
#include "apiTypesV2/Registration.h"                           // Registration
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/context/orionldCoreContext.h"                // ORIONLD_CORE_CONTEXT_URL
#include "orionld/serviceRoutines/orionldPostRegistrations.h"  // Own Interface
#include "orionld/common/orionldState.h"                       // orionldState
#include "mongoBackend/mongoRegistrationGet.h"                 // mongoRegistrationGet
#include "mongoBackend/mongoRegistrationCreate.h"              // mongoRegistrationCreate

#include "orionld/kjTree/kjTreeToRegistration.h"               // kjTreeToRegistration



// ----------------------------------------------------------------------------
//
// orionldPostRegistrations -
//
bool orionldPostRegistrations(ConnectionInfo* ciP)
{
 ngsiv2::Registration reg;
 std::string regId;
 OrionError oError;

  if (orionldState.contextP != NULL)
      reg.ldContext = orionldState.contextP->url;
  else
    reg.ldContext = ORIONLD_DEFAULT_CONTEXT_URL;

  char* regIdP = NULL;
  if (kjTreeToRegistration(ciP, &reg, &regIdP) == false)
  {
    LM_E(("kjTreeToRegistration FAILED"));
    // orionldErrorResponseCreate is invoked by kjTreeToRegistration
    return false;
  }
   //
  // Does the Registration already exist?
  //
  if (regIdP != NULL)
  {
    ngsiv2::Registration  registration;

    mongoRegistrationGet(&registration, regIdP, orionldState.tenant, ciP->servicePathV[0], &oError);

    if (!registration.id.empty())
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Registration already exists", regIdP, OrionldDetailsString);
      return false;
    }
  }

  //
  // Create the Registration
  //

  mongoRegistrationCreate(&reg,
                                  orionldState.tenant,
                                  ciP->servicePathV[0],
                                  &regId,
                                  &oError);

  ciP->httpStatusCode = SccCreated;

  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/registration/", regId.c_str());

  return true;
}
