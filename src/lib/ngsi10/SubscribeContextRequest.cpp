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
#include "ngsi/Request.h"
#include "ngsi/StatusCode.h"
#include "rest/EntityTypeInfo.h"
#include "ngsi10/SubscribeContextResponse.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "mongoBackend/dbConstants.h"
#include "alarmMgr/alarmMgr.h"


using namespace ngsiv2;



/* ****************************************************************************
*
* SubscribeContextRequest::check -
*/
std::string SubscribeContextRequest::check(const std::string& predetectedError, int counter)
{
  SubscribeContextResponse response;
  std::string              res;

  /* First, check optional fields only in the case they are present */
  /* Second, check the other (mandatory) fields */

  if (((res = entityIdVector.check(SubscribeContext))                                   != "OK") ||
      ((res = attributeList.check())                                                    != "OK") ||
      ((res = reference.check(SubscribeContext))                                        != "OK") ||
      ((res = duration.check())                                                         != "OK") ||
      ((res = restriction.check(restrictions))                                          != "OK") ||
      ((res = notifyConditionVector.check(SubscribeContext, predetectedError, counter)) != "OK") ||
      ((res = throttling.check())                                                       != "OK"))
  {
    alarmMgr.badInput(clientIp, res);
    response.subscribeError.errorCode.fill(SccBadRequest, std::string("invalid payload: ") + res);
    return response.toJsonV1();
  }

  return "OK";
}



/* ****************************************************************************
*
* SubscribeContextRequest::release -
*
*/
void SubscribeContextRequest::release(void)
{
  restriction.release();
  entityIdVector.release();
  attributeList.release();
  notifyConditionVector.release();
}



/* ****************************************************************************
*
* SubscribeContextRequest::fill -
*/
void SubscribeContextRequest::fill(EntityTypeInfo typeInfo)
{
  if ((typeInfo == EntityTypeEmpty) || (typeInfo == EntityTypeNotEmpty))
  {
    Scope* scopeP = new Scope(SCOPE_FILTER_EXISTENCE, SCOPE_VALUE_ENTITY_TYPE);

    scopeP->oper  = (typeInfo == EntityTypeEmpty)? SCOPE_OPERATOR_NOT : "";

    restriction.scopeVector.push_back(scopeP);
  }
}
