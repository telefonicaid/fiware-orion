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

#if 0
/* ****************************************************************************
*
* SubscribeContextRequest::render - 
*/
std::string SubscribeContextRequest::render(RequestType requestType, const std::string& indent)
{
  std::string  out                             = "";
  std::string  tag                             = "subscribeContextRequest";
  std::string  indent2                         = indent + "  ";

  bool         attributeListRendered           = attributeList.size() != 0;
  bool         referenceRendered               = true;  // Mandatory
  bool         durationRendered                = duration.get() != "";
  bool         restrictionRendered             = restrictions != 0;
  bool         notifyConditionVectorRendered   = notifyConditionVector.size() != 0;
  bool         throttlingRendered              = throttling.get() != "";

  bool         commaAfterThrottling            = false; // Last element;
  bool         commaAfterNotifyConditionVector = throttlingRendered;
  bool         commaAfterRestriction           = notifyConditionVectorRendered || throttlingRendered;
  bool         commaAfterDuration              = restrictionRendered || notifyConditionVectorRendered || throttlingRendered;
  bool         commaAfterReference             = durationRendered || restrictionRendered ||notifyConditionVectorRendered || throttlingRendered;
  bool         commaAfterAttributeList         = referenceRendered || durationRendered || restrictionRendered ||notifyConditionVectorRendered || throttlingRendered;
  bool         commaAfterEntityIdVector        = attributeListRendered || referenceRendered || durationRendered || restrictionRendered ||notifyConditionVectorRendered || throttlingRendered;

  out += startTag1(indent, tag, false);
  out += entityIdVector.render(indent2, commaAfterEntityIdVector);
  out += attributeList.render(indent2, commaAfterAttributeList);
  out += reference.render(indent2, commaAfterReference);
  out += duration.render(indent2, commaAfterDuration);
  out += restriction.render(indent2, restrictions, commaAfterRestriction);
  out += notifyConditionVector.render(indent2, commaAfterNotifyConditionVector);
  out += throttling.render(indent2, commaAfterThrottling);
  out += endTag(indent);

  return out;
}
#endif



/* ****************************************************************************
*
* SubscribeContextRequest::check - 
*/
std::string SubscribeContextRequest::check(ConnectionInfo* ciP, RequestType requestType, const std::string& indent, const std::string& predetectedError, int counter)
{
  SubscribeContextResponse response;
  std::string              res;

  /* First, check optional fields only in the case they are present */
  /* Second, check the other (mandatory) fields */

  if (((res = entityIdVector.check(ciP, SubscribeContext, indent, predetectedError, counter))        != "OK") ||
      ((res = attributeList.check(SubscribeContext, indent, predetectedError, counter))         != "OK") ||
      ((res = reference.check(SubscribeContext, indent, predetectedError, counter))             != "OK") ||
      ((res = duration.check(SubscribeContext, indent, predetectedError, counter))              != "OK") ||
      ((res = restriction.check(SubscribeContext, indent, predetectedError, restrictions))      != "OK") ||
      ((res = notifyConditionVector.check(SubscribeContext, indent, predetectedError, counter)) != "OK") ||
      ((res = throttling.check(SubscribeContext, indent, predetectedError, counter))            != "OK"))
  {
    alarmMgr.badInput(clientIp, res);
    response.subscribeError.errorCode.fill(SccBadRequest, std::string("invalid payload: ") + res);
    return response.render(SubscribeContext, indent);
  }

  return "OK";
}



/* ****************************************************************************
*
* SubscribeContextRequest::present - 
*/
void SubscribeContextRequest::present(const std::string& indent)
{
  entityIdVector.present(indent + "  ");
  attributeList.present(indent + "  ");
  reference.present(indent + "  ");
  duration.present(indent + "  ");
  restriction.present(indent + "  ");
  notifyConditionVector.present(indent + "  ");
  throttling.present(indent + "  ");
}



/* ****************************************************************************
*
* SubscribeContextRequest::release - 
*/
void SubscribeContextRequest::release(void)
{
  entityIdVector.release();
  attributeList.release();
  //restriction.release();    // FIXME PR: not truly sure if this is "memory-leak safe" but if enabled causes segfault
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
void  SubscribeContextRequest::toNgsiv2Subscription(Subscription* sub)
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
    duration.set(DEFAULT_DURATION);
  }

  sub->expires = duration.parse() + getCurrentTime();

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

  // Note we don't do anything with 'restrictions': it is not needed by the NGSIv2 logic

  // Fill NGSIv2 fields not existing in NGSIv1 with default values

  // status
  sub->status = STATUS_ACTIVE;

  // descriptionProvided
  sub->descriptionProvided = false;

  // attrsFormat
  sub->attrsFormat = NGSI_V1_LEGACY;

  // blaclist
  sub->notification.blackList = false;

  // extended
  sub->notification.httpInfo.extended = false;

  // description and expression are not touched, so default empty string provided by constructor will be used
}
