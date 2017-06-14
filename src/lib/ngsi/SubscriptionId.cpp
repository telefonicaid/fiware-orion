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
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/idCheck.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/SubscriptionId.h"



/* ****************************************************************************
*
* SubscriptionId::SubscriptionId -
*/
SubscriptionId::SubscriptionId()
{
  string = "";
}



/* ****************************************************************************
*
* SubscriptionId::SubscriptionId -
*/
SubscriptionId::SubscriptionId(const std::string& subId)
{
  string = subId;
}



/* ****************************************************************************
*
* SubscriptionId::check -
*/
std::string SubscriptionId::check
(
  RequestType         requestType,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string out = "OK";

  if (string != "")
  {
    out = idCheck(string);
  }

  return out;
}



/* ****************************************************************************
*
* SubscriptionId::isEmpty -
*/
bool SubscriptionId::isEmpty(void)
{
  return (string == "")? true : false;
}



/* ****************************************************************************
*
* SubscriptionId::set -
*/
void SubscriptionId::set(const std::string& value)
{
  string = value;
}



/* ****************************************************************************
*
* SubscriptionId::get -
*/
std::string SubscriptionId::get(void) const
{
  return string;
}



/* ****************************************************************************
*
* SubscriptionId::c_str -
*/
const char* SubscriptionId::c_str(void) const
{
  return string.c_str();
}



/* ****************************************************************************
*
* SubscriptionId::present -
*/
void SubscriptionId::present(const std::string& indent)
{
  if (string != "")
  {
    LM_T(LmtPresent, ("%sSubscriptionId: %s\n", 
		      indent.c_str(), 
		      string.c_str()));
  }
  else
  {
    LM_T(LmtPresent, ("%sNo SubscriptionId\n", indent.c_str()));
  }
}



/* ****************************************************************************
*
* SubscriptionId::render -
*/
void SubscriptionId::render
(
  rapidjson::Writer<rapidjson::StringBuffer>& writer,
  RequestType container
)
{
  std::string xString = string;

  if (xString == "")
  {
    if ((container == RtSubscribeContextAvailabilityResponse)          ||
        (container == RtUpdateContextAvailabilitySubscriptionResponse) ||
        (container == RtUnsubscribeContextAvailabilityResponse)        ||
        (container == NotifyContextAvailability)                       ||
        (container == UpdateContextSubscription)                       ||
        (container == UnsubscribeContext)                              ||
        (container == RtUnsubscribeContextResponse)                    ||
        (container == NotifyContext)                                   ||
        (container == RtSubscribeResponse)                             ||
        (container == RtSubscribeError))
    {
      // subscriptionId is Mandatory
      xString = "000000000000000000000000";
    }
    else
    {
      return;  // subscriptionId is Optional
    }
  }

  writer.Key("subscriptionId");
  writer.String(xString.c_str());
}



/* ****************************************************************************
*
* release -
*/
void SubscriptionId::release(void)
{
  /* This method is included for the sake of homogeneity */
  string = "";
}



/* ****************************************************************************
*
* SubscriptionId::rendered -
*/
bool SubscriptionId::rendered(RequestType container)
{
  if ((string == "") || (string == "000000000000000000000000"))
  {
    if ((container == RtSubscribeContextAvailabilityResponse)          ||
        (container == RtUpdateContextAvailabilitySubscriptionResponse) ||
        (container == RtUnsubscribeContextAvailabilityResponse)        ||
        (container == NotifyContextAvailability)                       ||
        (container == UpdateContextSubscription)                       ||
        (container == UnsubscribeContext)                              ||
        (container == RtUnsubscribeContextResponse)                    ||
        (container == NotifyContext)                                   ||
        (container == RtSubscribeResponse)                             ||
        (container == RtSubscribeError))
    {
      return true;
    }
    else
    {
      return false;  // subscriptionId is Optional
    }
  }

  return true;
}
