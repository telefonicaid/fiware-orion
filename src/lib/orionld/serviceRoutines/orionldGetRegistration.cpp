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
* Author: Ken Zangelin and Larysse Savanna
*/
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderAdd
#include "mongoBackend/mongoRegistrationGet.h"                 // mongoLdRegistrationGet
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldGetRegistration.h"    // Own Interface
#include "orionld/kjTree/kjTreeFromRegistration.h"             // kjTreeFromRegistration



// ----------------------------------------------------------------------------
//
// orionldGetRegistration -
//
bool orionldGetRegistration(ConnectionInfo* ciP)
{
  ngsiv2::Registration  registration;
  char*                 details;

  LM_T(LmtServiceRoutine, ("In orionldGetRegistration (%s)", orionldState.wildcard[0]));
  LM_TMP(("TENANT: %s", orionldState.tenant));

  if (mongoLdRegistrationGet(&registration, orionldState.wildcard[0], orionldState.tenant, &ciP->httpStatusCode, &details) != true)
  {
    LM_E(("mongoLdRegistrationGet error: %s", details));
    orionldErrorResponseCreate(OrionldResourceNotFound, details, orionldState.wildcard[0], OrionldDetailString);
    return false;
  }

  // Transform to KjNode tree
  ciP->httpStatusCode       = SccOk;
  orionldState.responseTree = kjTreeFromRegistration(ciP, &registration);

  return true;
}
