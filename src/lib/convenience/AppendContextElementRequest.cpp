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

#include "common/tag.h"
#include "convenience/AppendContextElementRequest.h"
#include "convenience/AppendContextElementResponse.h"
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/MetadataVector.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* Constructor - 
*/
AppendContextElementRequest::AppendContextElementRequest()
{
}



/* ****************************************************************************
*
* toJsonV1 -
*/
std::string AppendContextElementRequest::toJsonV1
(
  bool         asJsonObject,
  RequestType  requestType
)
{
  std::string out = "";

  out += startTag();

  if (entity.id != "")
  {
    out += entity.toJsonV1(false);
  }

  // No metadata filter in this case, an empty vector is used to fulfil method signature.
  // For attribute filter, we use the ContextAttributeVector itself
  std::vector<std::string> emptyMdV;

  out += contextAttributeVector.toJsonV1(asJsonObject, requestType, contextAttributeVector.vec, emptyMdV);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* check - 
*
*/
std::string AppendContextElementRequest::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  RequestType         requestType,
  const std::string&  predetectedError     // Predetected Error, normally during parsing
)
{
  AppendContextElementResponse  response;
  std::string                   res;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = contextAttributeVector.check(apiVersion, AppendContextElement)) != "OK")
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return response.toJsonV1(asJsonObject, requestType);
}



/* ****************************************************************************
*
* release - 
*/
void AppendContextElementRequest::release(void)
{
  contextAttributeVector.release();
}
