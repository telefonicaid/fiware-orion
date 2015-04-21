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

#include "common/Format.h"
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
  errorCode.tagSet("errorCode");
}



/* ****************************************************************************
*
* render -
*/
std::string UpdateContextElementResponse::render
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  const std::string&  indent
)
{
  std::string tag = "updateContextElementResponse";
  std::string out = "";

  out += startTag(indent, tag, ciP->outFormat, false);

  if ((errorCode.code != SccNone) && (errorCode.code != SccOk))
  {
    out += errorCode.render(ciP->outFormat, indent + "  ");
  }
  else
  {
    out += contextAttributeResponseVector.render(ciP, requestType, indent + "  ");
  }

  out += endTag(indent, tag, ciP->outFormat);

  return out;
}



/* ****************************************************************************
*
* check -
*/
std::string UpdateContextElementResponse::check
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  const std::string&  indent,
  const std::string&  predetectedError,  // Predetected Error, normally during parsing
  int                 counter
)
{
  std::string res;

  if (predetectedError != "")
  {
    errorCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = contextAttributeResponseVector.check(ciP, requestType, indent, "", counter)) != "OK")
  {
    errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return render(ciP, requestType, indent);
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
      cerP->contextElement.contextAttributeVector[aIx]->value = "";
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



/* ****************************************************************************
*
* present - 
*/
void UpdateContextElementResponse::present(const std::string& indent)
{
  LM_F(("%sUpdateContextElementResponse:", indent.c_str()));
  contextAttributeResponseVector.present(indent + "  ");
  errorCode.present(indent + "  ");
}
