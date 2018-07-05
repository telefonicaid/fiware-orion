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

#include "logMsg/traceLevels.h"
#include "common/tag.h"
#include "convenience/ContextAttributeResponse.h"
#include "convenience/UpdateContextElementResponse.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* UpdateContextElementResponse::UpdateContextElementResponse -
*/
UpdateContextElementResponse::UpdateContextElementResponse()
{
  errorCode.keyNameSet("errorCode");
}



/* ****************************************************************************
*
* render -
*/
std::string UpdateContextElementResponse::render
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  RequestType         requestType
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
    out += contextAttributeResponseVector.render(apiVersion, asJsonObject, requestType);
  }

  out += endTag();

  return out;
}



/* ****************************************************************************
*
* check -
*/
std::string UpdateContextElementResponse::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  RequestType         requestType,
  const std::string&  predetectedError  // Predetected Error, normally during parsing
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
* UpdateContextElementResponse::release -
*/
void UpdateContextElementResponse::release(void)
{
  contextAttributeResponseVector.release();
  errorCode.release();
}



/* ****************************************************************************
*
* UpdateContextElementResponse::fill -
*
* NOTE
* This method is used in the service routine of 'PUT /v1/contextEntities/{entityId::id}.
* Only ONE response in the vector contextElementResponseVector of UpdateContextResponse is possible.
*/
void UpdateContextElementResponse::fill(UpdateContextResponse* ucrsP)
{
  ContextElementResponse* cerP = ucrsP->contextElementResponseVector[0];

  errorCode.fill(ucrsP->errorCode);
  if (errorCode.code == SccNone)
  {
    errorCode.fill(SccOk, errorCode.details);
  }

  if (ucrsP->contextElementResponseVector.size() != 0)
  {
    //
    // Remove values from the context attributes
    //
    for (unsigned int aIx = 0; aIx < cerP->contextElement.contextAttributeVector.size(); ++aIx)
    {
      //
      // NOTE
      //   Only stringValue is cleared here (not numberValue nor boolValue, which are new for v2).
      //   This is OK for /v1, where all fields are strings.
      //   For /v2, we would need to reset the valueType to STRING as well, but since this function is used only
      //   in v1, this is not strictly necessary.
      //   However, it doesn't hurt, so that modification is included as well: 
      //     cerP->contextElement.contextAttributeVector[aIx]->valueType = orion::ValueTypeString
      //
      cerP->contextElement.contextAttributeVector[aIx]->stringValue = "";
      cerP->contextElement.contextAttributeVector[aIx]->valueType   = orion::ValueTypeString;
    }

    contextAttributeResponseVector.fill(&cerP->contextElement.contextAttributeVector, cerP->statusCode);
  }


  //
  // Special treatment if only one contextElementResponse that is NOT FOUND and if
  // UpdateContextElementResponse::errorCode is not 404 already
  //
  // Also if NO contextElementResponse is present
  //
  // These 'fixes' are mainly to maintain backward compatibility
  //
  if ((errorCode.code != SccContextElementNotFound) && (contextAttributeResponseVector.size() == 1) && (contextAttributeResponseVector[0]->statusCode.code == SccContextElementNotFound))
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
