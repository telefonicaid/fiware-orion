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
#include "ngsi/AttributeDomainName.h"
#include "ngsi/ContextAttributeVector.h"
#include "convenience/UpdateContextElementRequest.h"
#include "convenience/UpdateContextElementResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* render - 
*/
std::string UpdateContextElementRequest::render(ApiVersion apiVersion, bool asJsonObject, RequestType requestType)
{
  std::string out = "";

  out += startTag();
  out += attributeDomainName.render(true);
  out += contextAttributeVector.render(apiVersion, asJsonObject, requestType);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* check - 
*
*
* FIXME P3: once (if ever) AttributeDomainName::check stops to always return "OK", put back this piece of code 
*           in its place:
-
*   else if ((res = attributeDomainName.check(AppendContextElement, indent, predetectedError, counter)) != "OK")
*   {
*     response.errorCode.fill(SccBadRequest, res);
*   }
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

  if (predetectedError != "")
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

  return response.render(apiVersion, asJsonObject, requestType);
}



/* ****************************************************************************
*
* present - 
*/
void UpdateContextElementRequest::present(const std::string&  indent)
{
  attributeDomainName.present(indent);
  contextAttributeVector.present(indent);
}



/* ****************************************************************************
*
* release - 
*/
void UpdateContextElementRequest::release(void)
{
  contextAttributeVector.release();
}
