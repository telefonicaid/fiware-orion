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

#include "rapidjson/prettywriter.h"

#include "common/globals.h"
#include "ngsi/AttributeList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/Duration.h"
#include "ngsi/Reference.h"
#include "ngsi/Restriction.h"
#include "ngsi/SubscriptionId.h"
#include "ngsi9/SubscribeContextAvailabilityResponse.h"
#include "ngsi9/SubscribeContextAvailabilityRequest.h"

/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::SubscribeContextAvailabilityRequest
*
* Explicit constructor needed to initialize primitive types so they don't get
* random values from the stack
*/
SubscribeContextAvailabilityRequest::SubscribeContextAvailabilityRequest()
{
  restrictions = 0;
}

/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::render -
*/
std::string SubscribeContextAvailabilityRequest::render
(
  int indent
)
{
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  if (indent < 0)
  {
    indent = DEFAULT_JSON_INDENT;
  }
  writer.SetIndent(' ', indent);

  writer.StartObject();
  entityIdVector.toJson(writer);
  attributeList.toJson(writer);
  reference.toJson(writer);
  duration.toJson(writer);
  restriction.toJson(writer);
  writer.EndObject();

  return sb.GetString();
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::check -
*/
std::string SubscribeContextAvailabilityRequest::check(const std::string& indent, const std::string& predetectedError, int counter)
{
  SubscribeContextAvailabilityResponse response;
  std::string                          res;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = entityIdVector.check(SubscribeContextAvailability, indent))                              != "OK") ||
           ((res = attributeList.check(SubscribeContextAvailability, indent, predetectedError, counter))    != "OK") ||
           ((res = reference.check(SubscribeContextAvailability, indent, predetectedError, counter))        != "OK") ||
           ((res = duration.check(SubscribeContextAvailability, indent, predetectedError, counter))         != "OK") ||
           ((res = restriction.check(SubscribeContextAvailability, indent, predetectedError, restrictions)) != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.render();
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::release -
*/
void SubscribeContextAvailabilityRequest::release(void)
{
   entityIdVector.release();
   restriction.release();
   attributeList.release();
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::present -
*/
void SubscribeContextAvailabilityRequest::present(const std::string& indent)
{
   entityIdVector.present(indent);
   attributeList.present(indent);
   reference.present(indent);
   duration.present(indent);
   restriction.present(indent);
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::fill -
*/
void SubscribeContextAvailabilityRequest::fill(EntityTypeInfo typeInfo)
{
  if ((typeInfo == EntityTypeEmpty) || (typeInfo == EntityTypeNotEmpty))
  {
    Scope* scopeP = new Scope(SCOPE_FILTER_EXISTENCE, SCOPE_VALUE_ENTITY_TYPE);

    scopeP->oper  = (typeInfo == EntityTypeEmpty)? SCOPE_OPERATOR_NOT : "";

    restriction.scopeVector.push_back(scopeP);
  }
}
