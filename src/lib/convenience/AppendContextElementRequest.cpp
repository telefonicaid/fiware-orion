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
#include "convenience/AppendContextElementRequest.h"
#include "convenience/AppendContextElementResponse.h"
#include "ngsi/AttributeDomainName.h"
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
* render - 
*/
std::string AppendContextElementRequest::render(ConnectionInfo* ciP, RequestType requestType, std::string indent)
{
  std::string tag = "appendContextElementRequest";
  std::string out = "";

  out += startTag1(indent, tag, false);

  if (entity.id != "")
  {
    out += entity.render(indent + "  ");
  }

  out += attributeDomainName.render(indent + "  ", true);
  out += contextAttributeVector.render(ciP, requestType, indent + "  ");
  out += domainMetadataVector.render(indent + "  ");
  out += endTag(indent);

  return out;
}



/* ****************************************************************************
*
* check - 
*
* FIXME P3: once (if ever) AttributeDomainName::check stops to always return "OK", put back this piece of code 
*           in its place:
-
*   else if ((res = attributeDomainName.check(AppendContextElement, indent, predetectedError, counter)) != "OK")
*   {
*     response.errorCode.fill(SccBadRequest, res):
*   }
*
*/
std::string AppendContextElementRequest::check
(
  ConnectionInfo*  ciP,
  RequestType      requestType,
  std::string      indent,
  std::string      predetectedError,     // Predetected Error, normally during parsing
  int              counter
)
{
  AppendContextElementResponse  response;
  std::string                   res;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = contextAttributeVector.check(ciP, AppendContextElement, indent, predetectedError, counter)) != "OK")
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else if ((res = domainMetadataVector.check(ciP, AppendContextElement, indent, predetectedError, counter)) != "OK")
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return response.render(ciP, requestType, indent);
}



/* ****************************************************************************
*
* present - 
*/
void AppendContextElementRequest::present(std::string indent)
{
  attributeDomainName.present(indent);
  contextAttributeVector.present(indent);
  domainMetadataVector.present("Domain", indent);
}



/* ****************************************************************************
*
* release - 
*/
void AppendContextElementRequest::release(void)
{
  contextAttributeVector.release();
  domainMetadataVector.release();
}
