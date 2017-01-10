/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/tag.h"
#include "convenience/ContextAttributeResponseVector.h"
#include "ngsi/StatusCode.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/UpdateContextResponse.h"
#include "convenience/AppendContextElementResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* AppendContextElementResponse::AppendContextElementResponse - 
*/
AppendContextElementResponse::AppendContextElementResponse() : errorCode("errorCode")
{
}



/* ****************************************************************************
*
* AppendContextElementResponse::render - 
*/
std::string AppendContextElementResponse::render
(
  ApiVersion   apiVersion,
  bool         asJsonObject,
  RequestType  requestType
)
{
  std::string out = "";

  out += startTag();

  if ((errorCode.code != SccNone) && (errorCode.code != SccOk))
  {
    out += errorCode.render(false);
  }
  else
  {
    if (entity.id != "")
    {
      out += entity.render(true);
    }

    out += contextAttributeResponseVector.render(apiVersion, asJsonObject, requestType);
  }

  out += endTag();

  return out;
}



/* ****************************************************************************
*
* AppendContextElementResponse::check - 
*/
std::string AppendContextElementResponse::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  RequestType         requestType,
  const std::string&  predetectedError
)
{
  std::string res;

  if (predetectedError != "")
  {
    errorCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = contextAttributeResponseVector.check(apiVersion, asJsonObject, requestType, "")) != "OK")
  {
    errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return render(apiVersion, asJsonObject, requestType);
}



/* ****************************************************************************
*
* AppendContextElementResponse::release - 
*/
void AppendContextElementResponse::release(void)
{
  LM_T(LmtRelease, ("Releasing AppendContextElementResponse"));

  contextAttributeResponseVector.release();
  errorCode.release();
}



/* ****************************************************************************
*
* AppendContextElementResponse::fill - 
*
* NOTE
* This method is used in the service routine of 'POST /v1/contextEntities/{entityId::id} et al.
* Only ONE response in the vector contextElementResponseVector of UpdateContextResponse is possible.
*/
void AppendContextElementResponse::fill(UpdateContextResponse* ucrsP, const std::string& entityId, const std::string& entityType)
{
  if (ucrsP->contextElementResponseVector.size() != 0)
  {
    ContextElementResponse* cerP = ucrsP->contextElementResponseVector[0];

    contextAttributeResponseVector.fill(&cerP->contextElement.contextAttributeVector, cerP->statusCode);
    
    entity.fill(&cerP->contextElement.entityId);
  }
  else
  {
    entity.fill(entityId, entityType, "false");
  }

  errorCode.fill(ucrsP->errorCode);

  //
  // Special treatment if only one contextElementResponse that is NOT FOUND and if
  // AppendContextElementResponse::errorCode is not 404 already
  //
  // Also if NO contextElementResponse is present
  //
  // These 'fixes' are mainly to maintain backward compatibility
  //
  if ((errorCode.code != SccContextElementNotFound) &&
      (contextAttributeResponseVector.size() == 1) &&
      (contextAttributeResponseVector[0]->statusCode.code == SccContextElementNotFound)
     )
  {
    errorCode.fill(SccContextElementNotFound);
  }
  else if ((errorCode.code != SccContextElementNotFound) && (contextAttributeResponseVector.size() == 0))
  {
    errorCode.fill(SccContextElementNotFound);
  }
  else if (contextAttributeResponseVector.size() == 1)
  {
    //
    // Now, if any error inside ContextAttributeResponse, move it to the outside, but only if we have ONLY ONE contextAttributeResponse
    // and only if there is no error already in the 'external' errorCode.
    //
    if (((errorCode.code == SccNone) || (errorCode.code == SccOk)) && 
        ((contextAttributeResponseVector[0]->statusCode.code != SccNone) && (contextAttributeResponseVector[0]->statusCode.code != SccOk)))
    {
      errorCode.fill(contextAttributeResponseVector[0]->statusCode);
    }
  }

  // Now, if the external error code is 404 and 'details' is empty - add the name of the incoming entity::id as details
  if ((errorCode.code == SccContextElementNotFound) && (errorCode.details == ""))
  {
    if (ucrsP->contextElementResponseVector.size() == 1)
    {
      errorCode.details = ucrsP->contextElementResponseVector[0]->contextElement.entityId.id;
    }
  }
}
