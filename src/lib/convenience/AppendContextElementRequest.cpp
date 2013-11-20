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
* fermin at tid dot es
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
std::string AppendContextElementRequest::render(Format format, std::string indent)
{
  std::string tag = "appendContextElementRequest";
  std::string out = "";

  out += startTag(indent, tag, format);
  out += attributeDomainName.render(format, indent + "  ");
  out += contextAttributeVector.render(format, indent + "  ");
  out += domainMetadataVector.render(format, indent + "  ");
  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* check - 
*
* FIXME P3: once (if ever) AttributeDomainName::check stops to always reutnr "OK", put back this piece of code 
*           in its place:
-
*   else if ((res = attributeDomainName.check(AppendContextElement, format, indent, predetectedError, counter)) != "OK")
*   {
*     response.errorCode.code         = SccBadRequest;
*     response.errorCode.reasonPhrase = res;
*   }
*
*/
std::string AppendContextElementRequest::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
   AppendContextElementResponse  response;
   std::string                   res;

   if (predetectedError != "")
   {
     response.errorCode.code         = SccBadRequest;
     response.errorCode.reasonPhrase = predetectedError;
   }
   else if ((res = contextAttributeVector.check(AppendContextElement, format, indent, predetectedError, counter)) != "OK")
   {
     response.errorCode.code         = SccBadRequest;
     response.errorCode.reasonPhrase = res;
   }
   else if ((res = domainMetadataVector.check(AppendContextElement, format, indent, predetectedError, counter)) != "OK")
   {
     response.errorCode.code         = SccBadRequest;
     response.errorCode.reasonPhrase = res;
   }
   else
     return "OK";
   
   return response.render(format, indent);
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
