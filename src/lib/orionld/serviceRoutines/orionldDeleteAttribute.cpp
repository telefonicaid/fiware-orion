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
#include "rest/HttpStatusCode.h"                                 // SccContextElementNotFound
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/httpStatusCodeToOrionldErrorType.h"     // httpStatusCodeToOrionldErrorType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/mongoBackend/mongoAttributeExists.h"           // mongoAttributeExists
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"      // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteAttribute -
//
bool orionldDeleteAttribute(ConnectionInfo* ciP)
{
  char*   attrNameP;

  if ((strncmp(orionldState.wildcard[1], "http://", 7) == 0) || (strncmp(orionldState.wildcard[1], "https://", 8) == 0))
    attrNameP = orionldState.wildcard[1];
  else
    attrNameP = orionldContextItemExpand(orionldState.contextP, orionldState.wildcard[1], NULL, true, NULL);

  //
  // Does the attribute to be deleted even exist?
  //

  //
  // FIXME: Extra call to mongo - can this be avoided?
  //        By looking at the error code from the delete operation in mongo ...
  //
  if (mongoAttributeExists(orionldState.wildcard[0], attrNameP, orionldState.tenant) == false)
  {
    ciP->httpStatusCode = SccContextElementNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute Not Found", orionldState.wildcard[1]);
    return false;
  }

  // Create and fill in attribute and entity
  ContextAttribute*  caP = new ContextAttribute;
  Entity             entity;

  entity.id = orionldState.wildcard[0];
  caP->name = attrNameP;
  entity.attributeVector.push_back(caP);

  LM_T(LmtServiceRoutine, ("Deleting attribute '%s' of entity '%s'", orionldState.wildcard[1], orionldState.wildcard[0]));

  UpdateContextRequest  ucr;
  UpdateContextResponse ucResponse;

  ucr.fill(&entity, ActionTypeDelete);
  ciP->httpStatusCode = mongoUpdateContext(&ucr,
                                           &ucResponse,
                                           orionldState.tenant,
                                           ciP->servicePathV,
                                           ciP->uriParam,
                                           ciP->httpHeaders.xauthToken,
                                           ciP->httpHeaders.correlator,
                                           ciP->httpHeaders.ngsiv2AttrsFormat,
                                           ciP->apiVersion,
                                           NGSIV2_NO_FLAVOUR);

  if (ciP->httpStatusCode != SccOk)
  {
    orionldErrorResponseCreate(httpStatusCodeToOrionldErrorType(ciP->httpStatusCode), "DELETE /ngsi-ld/v1/entities/*/attrs/*", orionldState.wildcard[0]);
    ucr.release();

    return false;
  }

  ucr.release();
  ciP->httpStatusCode = SccNoContent;

  return true;
}
