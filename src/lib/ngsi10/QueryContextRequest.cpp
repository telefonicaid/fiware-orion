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
#include "rest/EntityTypeInfo.h"
#include "apiTypesV2/BatchQuery.h"



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
std::string QueryContextRequest::render(const std::string& indent)
{
  std::string   out                      = "";
  bool          attributeListRendered    = attributeList.size() != 0;
  bool          restrictionRendered      = restrictions != 0;
  bool          commaAfterAttributeList  = restrictionRendered;
  bool          commaAfterEntityIdVector = attributeListRendered || restrictionRendered;

  out += startTag(indent);
  out += entityIdVector.render(indent + "  ", commaAfterEntityIdVector);
  out += attributeList.render( indent + "  ", commaAfterAttributeList);
  out += restriction.render(   indent + "  ", restrictions, false);
  out += endTag(indent);

  return out;
}



/* ****************************************************************************
*
* QueryContextRequest::check -
*/
std::string QueryContextRequest::check(ApiVersion apiVersion, bool asJsonObject, const std::string& indent, const std::string& predetectedError)
{
  std::string           res;
  QueryContextResponse  response;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = entityIdVector.check(QueryContext, indent))                                 != "OK") ||
           ((res = attributeList.check(QueryContext,  indent, predetectedError, 0))            != "OK") ||
           ((res = restriction.check(QueryContext,    indent, predetectedError, restrictions)) != "OK"))
  {
    alarmMgr.badInput(clientIp, res);
    response.errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return response.render(apiVersion, asJsonObject, indent);
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



/* ****************************************************************************
*
* QueryContextRequest::fill -
*
* NOTE
* If the incoming bqP->entities.vec is empty, then one almighty entity::id is
* added to the QueryContextRequest::entityIdVector, namely, idPattern .* with empty type,
* matching ALL entities.
*
*/
void QueryContextRequest::fill(BatchQuery* bqP)
{
  if (bqP->entities.vec.size() != 0)
  {
    entityIdVector.fill(bqP->entities.vec);
  }
  else
  {
    EntityId* eP = new EntityId(".*", "", "true");
    entityIdVector.push_back(eP);
  }

  attributeList.fill(bqP->attributeV.attributeV);
  restriction.scopeVector.fill(bqP->scopeV, false);  // false: DO NOT ALLOCATE NEW scopes - reference the 'old' ones
  bqP->scopeV.vec.clear();  // QueryContextRequest::restriction.scopeVector has taken over the Scopes from bqP
}
