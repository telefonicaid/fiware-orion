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
#include <string>
#include <vector>

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/urlCheck.h"                             // urlCheck
#include "orionld/common/urnCheck.h"                             // urnCheck
#include "orionld/serviceRoutines/orionldDeleteEntity.h"         // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteEntity -
//
bool orionldDeleteEntity(ConnectionInfo* ciP)
{
  LM_T(LmtServiceRoutine, ("In orionldDeleteEntity"));

  // Check that the Entity ID is a valid URI
  char* details;

  if ((urlCheck(orionldState.wildcard[0], &details) == false) && (urnCheck(orionldState.wildcard[0], &details) == false))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity ID", details);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  // Fill in mongoRequest with the entity-id from the URL and Deleteas Action Type
  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement         ce;
  HttpStatusCode         status;

  ce.entityId.id = orionldState.wildcard[0];
  mongoRequest.contextElementVector.push_back(&ce);
  mongoRequest.updateActionType = ActionTypeDelete;

  // Call mongoBackend
  status = mongoUpdateContext(&mongoRequest,
                              &mongoResponse,
                              orionldState.tenant,
                              ciP->servicePathV,
                              ciP->uriParam,
                              ciP->httpHeaders.xauthToken,
                              ciP->httpHeaders.correlator,
                              ciP->httpHeaders.ngsiv2AttrsFormat,
                              ciP->apiVersion,
                              NGSIV2_NO_FLAVOUR);

  // Check result
  if (status != SccOk)
    LM_E(("mongoUpdateContext: %d", status));

  if (mongoResponse.oe.code != SccNone)
  {
    OrionldResponseErrorType eType = (mongoResponse.oe.code == SccContextElementNotFound)? OrionldResourceNotFound : OrionldBadRequestData;

    orionldErrorResponseCreate(eType, mongoResponse.oe.details.c_str(), orionldState.wildcard[0]);
    ciP->httpStatusCode = (mongoResponse.oe.code == SccContextElementNotFound)? SccContextElementNotFound : SccBadRequest;

    return false;
  }

  // HTTP Response Code is 204 - No Content
  ciP->httpStatusCode = SccNoContent;

  return true;
}
