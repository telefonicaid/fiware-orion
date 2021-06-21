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

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/ContextAttributeVector.h"
#include "convenience/UpdateContextElementRequest.h"
#include "convenience/UpdateContextElementResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* toJsonV1 -
*
*/
std::string UpdateContextElementRequest::toJsonV1(bool asJsonObject, RequestType requestType)
{
  std::string out = "";

  // No metadata filter in this case, an empty vector is used to fulfil method signature.
  // For attribute filter, we use the ContextAttributeVector itself
  std::vector<std::string> emptyMdV;

  out += startTag();
  out += contextAttributeVector.toJsonV1(asJsonObject, requestType, contextAttributeVector.vec, emptyMdV);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* check - 
*
*/
std::string UpdateContextElementRequest::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  RequestType         requestType,
  const std::string&  predetectedError     // Predetected Error, normally during parsing
)
{
  UpdateContextElementResponse  response;
  std::string                   res;

  if (!predetectedError.empty())
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }  
  else if ((res = contextAttributeVector.check(apiVersion, UpdateContextElement)) != "OK")
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
void UpdateContextElementRequest::release(void)
{
  contextAttributeVector.release();
}
