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
#include "mongoBackend/mongoQueryContext.h"                    // mongoQueryContext

#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/context/orionldContextAdd.h"                 // Add a context to the context list
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"     // kjTreeFromQueryContextResponse
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldGetEntity.h"          // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetEntity -
//
bool orionldGetEntity(ConnectionInfo* ciP)
{
  QueryContextRequest   request;
  EntityId              entityId(ciP->wildcard[0], "", "false", false);
  QueryContextResponse  response;

  LM_T(LmtServiceRoutine, ("In orionldGetEntity: %s", ciP->wildcard[0]));

  request.entityIdVector.push_back(&entityId);
  LM_TMP(("Calling mongoQueryContext"));
  //
  // FIXME: mongoQueryContext should respond with a KJson tree -
  //        next year perhaps, and when starting with new mongo driver
  //
  ciP->httpStatusCode = mongoQueryContext(&request,
                                          &response,
                                          ciP->tenant,
                                          ciP->servicePathV,
                                          ciP->uriParam,
                                          ciP->uriParamOptions,
                                          NULL,
                                          ciP->apiVersion);
  LM_TMP(("Back from mongoQueryContext. httpStatusCode == %d", ciP->httpStatusCode));

  if (response.errorCode.code == SccBadRequest)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Bad Request", NULL, OrionldDetailsString);
    return false;
  }

  //
  // Create response by converting "QueryContextResponse response" into a KJson tree
  // But first, check for "404 Not Found"
  //
  LM_TMP(("response.contextElementResponseVector.size: %d", response.contextElementResponseVector.size()));
  
  ciP->responseTree = kjTreeFromQueryContextResponse(ciP, &response);
  if (ciP->responseTree == NULL)
  {
    ciP->httpStatusCode = SccContextElementNotFound;
  }

  // request.entityIdVector.vec.erase(0);  // Remove 'entityId' from entityIdVector
  
  return true;
}
