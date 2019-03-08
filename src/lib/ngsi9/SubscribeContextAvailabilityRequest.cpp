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

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/StringList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/Duration.h"
#include "ngsi/Reference.h"
#include "ngsi/Restriction.h"
#include "ngsi/StatusCode.h"
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
* SubscribeContextAvailabilityRequest::toJsonV1 -
*/
std::string SubscribeContextAvailabilityRequest::toJsonV1(void)
{
  std::string out                      = "";
  bool        commaAfterEntityIdVector = (restrictions > 0) || !duration.isEmpty() || !reference.isEmpty() || (attributeList.size() != 0);
  bool        commaAfterAttributeList  = (restrictions > 0) || !duration.isEmpty() || !reference.isEmpty();
  bool        commaAfterReference      = (restrictions > 0) || !duration.isEmpty();
  bool        commaAfterDuration       = restrictions > 0;

  out += startTag();
  out += entityIdVector.toJsonV1(commaAfterEntityIdVector);
  out += attributeList.toJsonV1(commaAfterAttributeList, "attributes");
  out += reference.toJsonV1(commaAfterReference);
  out += duration.toJsonV1(commaAfterDuration);
  out += restriction.toJsonV1(restrictions, true);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::check -
*/
std::string SubscribeContextAvailabilityRequest::check(const std::string& predetectedError)
{
  SubscribeContextAvailabilityResponse response;
  std::string                          res;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = entityIdVector.check(SubscribeContextAvailability)) != "OK") ||
           ((res = attributeList.check())                              != "OK") ||
           ((res = reference.check(SubscribeContextAvailability))      != "OK") ||
           ((res = duration.check())                                   != "OK") ||
           ((res = restriction.check(restrictions))                    != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.toJsonV1();
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
