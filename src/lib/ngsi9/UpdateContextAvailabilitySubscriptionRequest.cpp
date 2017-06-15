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
#include <vector>

#include "rapidjson/prettywriter.h"

#include "common/globals.h"
#include "ngsi/Request.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionRequest.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"



/* ****************************************************************************
*
* Constructor -
*/
UpdateContextAvailabilitySubscriptionRequest::UpdateContextAvailabilitySubscriptionRequest()
{
   restrictions = 0;
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest::render -
*/
std::string UpdateContextAvailabilitySubscriptionRequest::render
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
  duration.toJson(writer);
  restriction.toJson(writer);
  subscriptionId.toJson(writer, UpdateContextAvailabilitySubscription);

  return sb.GetString();
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest::present -
*/
void UpdateContextAvailabilitySubscriptionRequest::present(const std::string& indent)
{
   entityIdVector.present(indent + "  ");
   attributeList.present(indent + "  ");
   duration.present(indent + "  ");
   restriction.present(indent + "  ");
   subscriptionId.present(indent + "  ");
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest::check -
*/
std::string UpdateContextAvailabilitySubscriptionRequest::check(const std::string& indent, const std::string& predetectedError, int counter)
{
  std::string                                    res;
  UpdateContextAvailabilitySubscriptionResponse  response;

  response.subscriptionId = subscriptionId;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = entityIdVector.check(UpdateContextAvailabilitySubscription, indent))                                 != "OK") ||
           ((res = attributeList.check(UpdateContextAvailabilitySubscription, indent, predetectedError, counter))       != "OK") ||
           ((res = duration.check(UpdateContextAvailabilitySubscription, indent, predetectedError, counter))            != "OK") ||
           ((res = restriction.check(UpdateContextAvailabilitySubscription, indent, predetectedError, counter))         != "OK") ||
           ((res = subscriptionId.check(UpdateContextAvailabilitySubscription, indent, predetectedError, counter))      != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.render();
}



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionRequest::release -
*/
void UpdateContextAvailabilitySubscriptionRequest::release(void)
{
  entityIdVector.release();
  attributeList.release();
  duration.release();
  restriction.release();
  subscriptionId.release();
}
