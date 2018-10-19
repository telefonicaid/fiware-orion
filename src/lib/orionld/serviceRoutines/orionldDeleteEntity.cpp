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
#include "ngsi/ParseData.h"                                    // ParseData needed for postUpdateContext()
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "serviceRoutines/postUpdateContext.h"                 // postUpdateContext
#include "orionld/serviceRoutines/orionldDeleteEntity.h"       // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteEntity -
//
bool orionldDeleteEntity(ConnectionInfo* ciP)
{
  ParseData  parseData;
  Entity     entity;

  LM_T(LmtServiceRoutine, ("In orionldDeleteEntity"));

  // Check that the Entity ID is a valid URI
  char* details;

  if ((urlCheck(ciP->wildcard[0], &details) == false) && (urnCheck(ciP->wildcard[0], &details)))
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Entity ID", details, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  // Fill in entity with the entity-id from the URL
  entity.id = ciP->wildcard[0];

  // Fill in the upcr field for postUpdateContext, with the entity and DELETE as action
  parseData.upcr.res.fill(&entity, ActionTypeDelete);

  // Call standard op postUpdateContext
  std::vector<std::string> compV;  // dum my - postUpdateContext requires this arg as its 3rd parameter
  postUpdateContext(ciP, 3, compV, &parseData);

  // Check result
  if (parseData.upcrs.res.oe.code != SccNone)
  {
    OrionldResponseErrorType eType = (parseData.upcrs.res.oe.code == SccContextElementNotFound)? OrionldResourceNotFound : OrionldBadRequestData;

    orionldErrorResponseCreate(ciP, eType, parseData.upcrs.res.oe.details.c_str(), ciP->wildcard[0], OrionldDetailsString);
    ciP->httpStatusCode = (parseData.upcrs.res.oe.code == SccContextElementNotFound)? SccContextElementNotFound : SccBadRequest;

    // Release allocated data
    parseData.upcr.res.contextElementVector.release();

    return false;
  }

  // Release allocated data
  parseData.upcr.res.contextElementVector.release();

  return true;
}
