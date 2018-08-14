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
#include "logMsg/logMsg.h"

#include "rest/ConnectionInfo.h"
#include "orionld/serviceRoutines/orionldPostRegistrations.h"



// ----------------------------------------------------------------------------
//
// orionldPostRegistrations -
//
bool orionldPostRegistrations(ConnectionInfo* ciP)
{
  LM_TMP(("In orionldPostRegistrations"));

  ciP->responsePayload = (char*) "{ \"error\": \"not implemented\", \"details\": \"POST /ngsi-ld/v1/registrations\" }\n";
  ciP->httpStatusCode  = SccNotImplemented;

  return true;
}
