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
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldPostEntity.h"         // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPostEntity -
//
bool orionldPostEntity(ConnectionInfo* ciP)
{
  LM_T(LmtServiceRoutine, ("In orionldPostEntities: request tree at %p", ciP->requestTree));

  orionldErrorResponseCreate(ciP, OrionldBadRequestData, "not implemented - POST /ngsi-ld/v1/entities/*/attrs", ciP->wildcard[0]);

  ciP->httpStatusCode = SccNotImplemented;

  return true;
}
