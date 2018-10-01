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

#include "ngsi/StatusCode.h"
#include "ngsi/Request.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/StringList.h"
#include "ngsi/Restriction.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"

/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::DiscoverContextAvailabilityRequest
*
* Explicit constructor needed to initialize primitive types so they don't get
* random values from the stack
*/
DiscoverContextAvailabilityRequest::DiscoverContextAvailabilityRequest()
{
  restrictions = 0;
}

/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::release -
*/
void DiscoverContextAvailabilityRequest::release(void)
{
  entityIdVector.release();
  attributeList.release();
  restriction.release();
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::check -
*/
std::string DiscoverContextAvailabilityRequest::check(const std::string& predetectedError)
{
  DiscoverContextAvailabilityResponse  response;
  std::string                          res;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (entityIdVector.size() == 0)
  {
    response.errorCode.fill(SccContextElementNotFound);
  }
  else if (((res = entityIdVector.check(DiscoverContextAvailability))       != "OK") ||
           ((res = attributeList.check())                                   != "OK") ||
           ((restrictions != 0) && ((res = restriction.check(restrictions)) != "OK")))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.toJsonV1();
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::fill -
*/
void DiscoverContextAvailabilityRequest::fill
(
  EntityId&                            eid,
  const std::vector<std::string>&      attributeV,
  const Restriction&                   restriction
)
{
  entityIdVector.push_back(&eid);

  for (unsigned int ix = 0; ix < attributeV.size(); ++ix)
  {
    attributeList.push_back(attributeV[ix]);
  }

  // FIXME P9: restriction with scope-vector must be copied to this->restriction
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::fill -
*/
void DiscoverContextAvailabilityRequest::fill(const std::string& entityId, const std::string& entityType)
{
  EntityId* eidP = new EntityId(entityId, entityType, "false");

  entityIdVector.push_back(eidP);
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::fill -
*/
void DiscoverContextAvailabilityRequest::fill
(
  const std::string&  entityId,
  const std::string&  entityType,
  EntityTypeInfo      typeInfo,
  const std::string&  attributeName
)
{
  EntityId* eidP = new EntityId(entityId, entityType, "false");

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
