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
* Author: Ken Zangelin and Larysse Savanna
*/
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderAdd
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldGetRegistration.h"    // Own Interface
#include "orionld/mongoBackend/mongoLdRegistrationGet.h"       // mongoLdRegistrationGet
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

  if (mongoLdRegistrationGet(&registration, orionldState.wildcard[0], orionldState.tenant, &ciP->httpStatusCode, &details) != true)
  {
    LM_E(("mongoLdRegistrationGet error: %s", details));
    orionldErrorResponseCreate(OrionldResourceNotFound, details, orionldState.wildcard[0]);
    return false;
  }

  // Transform to KjNode tree
  ciP->httpStatusCode       = SccOk;
  orionldState.responseTree = kjTreeFromRegistration(ciP, &registration);

  return true;
}
