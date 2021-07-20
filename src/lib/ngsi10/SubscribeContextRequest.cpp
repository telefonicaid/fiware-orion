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
      ((res = throttling.check())                                                       != "OK") ||
      ((res = maxFailsLimit.check())                                                    != "OK"))
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
* Old versions of this method also include a 'restriction.release()' call. However, now each time
* a SubscribeContextRequest is created, the method toNgsiv2Subscription() is used on it and the
* 'ownership' of the Restriction is transferred to the corresponding NGSIv2 class. Thus, leaving
* that 'restriction.release()' would cause double-free problems.
*
* What causes the problem is the following line in SubscribeContextRequest::toNgsiv2Subscription:
*
*  sub->restriction = restriction;
*
* After doing this, we have TWO vectors pointing to the same scopes.
*
*/
void SubscribeContextRequest::release(void)
{
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


/* ****************************************************************************
*
* SubscribeContextRequest::toNgsiv2Subscription -
*/
void SubscribeContextRequest::toNgsiv2Subscription(Subscription* sub)
{
  // Convert entityIdVector
  for (unsigned int ix = 0; ix < entityIdVector.size(); ++ix)
  {
    EntityId* enP = entityIdVector[ix];
    EntID en;

    if (enP->isPatternIsTrue())
    {
      en.idPattern = enP->id;
    }
    else
    {
      en.id = enP->id;
    }
    en.type = enP->type;

    sub->subject.entities.push_back(en);
  }

  // Convert attributeList
  for (unsigned int ix = 0; ix < attributeList.size(); ++ix)
  {
    sub->notification.attributes.push_back(attributeList[ix]);
  }

  // Convert reference
  sub->notification.httpInfo.url = reference.get();

  // Convert duration
  if (duration.isEmpty())
  {
    sub->expires = DEFAULT_DURATION_IN_SECONDS + getCurrentTime();
  }
  else
  {
    sub->expires = duration.parse() + getCurrentTime();
  }

  // Convert restriction
  sub->restriction = restriction;

  // Convert notifyConditionVector
  for (unsigned int ix = 0; ix < notifyConditionVector.size(); ++ix)
  {
      NotifyCondition* ncP = notifyConditionVector[ix];
      if (ncP->type == ON_CHANGE_CONDITION)    // this is just a sanity measure: all types should be ONCHANGE
      {
        for (unsigned int jx = 0; jx < ncP->condValueList.size(); ++jx)
        {
          sub->subject.condition.attributes.push_back(ncP->condValueList[jx]);
        }
      }
  }

  // Convert throttling
  sub->throttling = throttling.parse();

  // Note that we don't do anything with 'restrictions': it is not needed by the NGSIv2 logic

  // Fill NGSIv2 fields not used in NGSIv1 with default values
  // description and expression are not touched, so default empty string provided by constructor will be used
  sub->status                         = STATUS_ACTIVE;
  sub->descriptionProvided            = false;
  sub->attrsFormat                    = NGSI_V1_LEGACY;
  sub->notification.blacklist         = false;
  sub->notification.httpInfo.custom   = false;

  sub->notification.metadata.clear();
}
