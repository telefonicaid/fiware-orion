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

#include "logMsg/logMsg.h"
#include "common/globals.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/Request.h"
#include "ngsi/AttributeList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/Restriction.h"
#include "ngsi10/QueryContextResponse.h"
#include "ngsi10/QueryContextRequest.h"
#include "rest/ConnectionInfo.h"
#include "rest/EntityTypeInfo.h"



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
* QueryContextRequest::QueryContextRequest
*/
QueryContextRequest::QueryContextRequest(const std::string& _contextProvider, EntityId* eP, const std::string& attributeName)
{
  contextProvider = _contextProvider;

  entityIdVector.push_back(new EntityId(eP));

  if (attributeName != "")
  {
    attributeList.push_back(attributeName);
  }

  restrictions = 0;
}



/* ****************************************************************************
*
* QueryContextRequest::QueryContextRequest
*/
QueryContextRequest::QueryContextRequest(const std::string& _contextProvider, EntityId* eP, const AttributeList& _attributeList)
{
  contextProvider = _contextProvider;

  entityIdVector.push_back(new EntityId(eP));
  
  attributeList.clone(_attributeList);

  restrictions = 0;
}



/* ****************************************************************************
*
* QueryContextRequest::render - 
*/
std::string QueryContextRequest::render(RequestType requestType, Format format, const std::string& indent)
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
std::string QueryContextRequest::check(ConnectionInfo* ciP, RequestType requestType, const std::string& indent, const std::string& predetectedError, int counter)
{
  std::string           res;
  QueryContextResponse  response;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = entityIdVector.check(ciP, QueryContext, ciP->outFormat, indent, predetectedError, 0))            != "OK") ||
           ((res = attributeList.check(QueryContext,  ciP->outFormat, indent, predetectedError, 0))            != "OK") ||
           ((res = restriction.check(QueryContext,    ciP->outFormat, indent, predetectedError, restrictions)) != "OK"))
  {
    alarmMgr.badInput(clientIp, res);
    response.errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return response.render(ciP, QueryContext, indent);
}



/* ****************************************************************************
*
* QueryContextRequest::present - 
*/
void QueryContextRequest::present(const std::string& indent)
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
void QueryContextRequest::fill(const std::string& entityId, const std::string& entityType, const std::string& attributeName)
{
  EntityId* eidP = new EntityId(entityId, entityType, "true");

  entityIdVector.push_back(eidP);

  if (attributeName != "")
  {
    attributeList.push_back(attributeName);
  }
}



/* ****************************************************************************
*
* QueryContextRequest::fill - 
*/
void QueryContextRequest::fill
(
  const std::string& entityId,
  const std::string& entityType,
  const std::string& isPattern,
  EntityTypeInfo     typeInfo,
  const std::string& attributeName
)
{
  EntityId* eidP = new EntityId(entityId, entityType, isPattern);

  entityIdVector.push_back(eidP);

  if ((typeInfo == EntityTypeEmpty) || (typeInfo == EntityTypeNotEmpty))
  {
    Scope* scopeP = new Scope(SCOPE_FILTER_EXISTENCE, SCOPE_VALUE_ENTITY_TYPE);

    scopeP->oper  = (typeInfo == EntityTypeEmpty)? SCOPE_OPERATOR_NOT : "";
      
    restriction.scopeVector.push_back(scopeP);
  }

  if (attributeName != "")
  {
    attributeList.push_back(attributeName);
  }
}
