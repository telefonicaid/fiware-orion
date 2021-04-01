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
#include "logMsg/logMsg.h"

#include "rest/ConnectionInfo.h"
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/rest/OrionLdRestService.h"                     // OrionLdRestService
#include "orionld/serviceRoutines/orionldGetTemporalEntities.h"  // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetTemporalEntities -
//
bool orionldGetTemporalEntities(ConnectionInfo* ciP)
{
  orionldState.httpStatusCode = 501;
  orionldState.noLinkHeader   = true;  // We don't want the Link header for non-implemented requests

  orionldErrorResponseCreate(OrionldOperationNotSupported, "Not Implemented in Orion-LD, please use Mintaka for this operation", orionldState.serviceP->url);
  return false;
}
