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

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/AttributeList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/Restriction.h"
#include "ngsi10/QueryContextResponse.h"
#include "ngsi10/QueryContextRequest.h"

/* ****************************************************************************
*
* QueryContextRequest::QueryContextRequest
*
* Explicit constructor needed to initialize primitive types so they don't get
* random values from the stack
*/
QueryContextRequest::QueryContextRequest()
{
  restrictions = 0;
}


/* ****************************************************************************
*
* QueryContextRequest::render - 
*/
std::string QueryContextRequest::render(RequestType requestType, Format format, std::string indent)
{
  std::string   out                      = "";
  std::string   tag                      = "queryContextRequest";
  bool          attributeListRendered    = attributeList.size() != 0;
  bool          restrictionRendered      = restrictions != 0;
  bool          commaAfterAttributeList  = restrictionRendered;
  bool          commaAfterEntityIdVector = attributeListRendered || restrictionRendered;

  out += startTag(indent, tag, format, false);
  out += entityIdVector.render(format, indent + "  ", commaAfterEntityIdVector);
  out += attributeList.render(format,  indent + "  ", commaAfterAttributeList);
  out += restriction.render(format,    indent + "  ", restrictions, false);
  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* QueryContextRequest::check - 
*/
std::string QueryContextRequest::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  std::string           res;
  QueryContextResponse  response;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = entityIdVector.check(QueryContext, format, indent, predetectedError, 0))         != "OK") ||
           ((res = attributeList.check(QueryContext, format, indent, predetectedError, 0))          != "OK") ||
           ((res = restriction.check(QueryContext, format, indent, predetectedError, restrictions)) != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.render(QueryContext, format, indent);
}



/* ****************************************************************************
*
* QueryContextRequest::present - 
*/
void QueryContextRequest::present(std::string indent)
{
  entityIdVector.present(indent);
  attributeList.present(indent);
  restriction.present(indent);
}



/* ****************************************************************************
*
* QueryContextRequest::release - 
*/
void QueryContextRequest::release(void)
{
  entityIdVector.release();
  restriction.release();
}



/* ****************************************************************************
*
* QueryContextRequest::fill - 
*/
void QueryContextRequest::fill(std::string entityId, std::string entityType, std::string attributeName)
{
  EntityId* eidP = new EntityId(entityId, entityType, "true");

  entityIdVector.push_back(eidP);

  if (attributeName != "")
    attributeList.push_back(attributeName);
}
