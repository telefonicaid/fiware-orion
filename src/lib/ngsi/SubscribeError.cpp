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

#include "common/tag.h"
#include "ngsi/StatusCode.h"
#include "ngsi/Request.h"
#include "ngsi/SubscribeError.h"



/* ****************************************************************************
*
* SubscribeError::SubscribeError -
*/
SubscribeError::SubscribeError()
{
  errorCode.keyNameSet("errorCode");
}



/* ****************************************************************************
*
* SubscribeError::render -
*/
std::string SubscribeError::render(RequestType requestType, const std::string& indent, bool comma)
{
  std::string out = "";

  out += startTag(indent, "subscribeError", false);

  // subscriptionId is Mandatory if part of updateContextSubscriptionResponse
  // errorCode is Mandatory so, the JSON comma is always TRUE
  if (requestType == UpdateContextSubscription)
  {
    //
    // NOTE: the subscriptionId must have come from the request.
    //       If the field is empty, we are in unit tests and I here set it to all zeroes
    //
    if (subscriptionId.get() == "")
    {
      subscriptionId.set("000000000000000000000000");
    }
    out += subscriptionId.render(requestType, indent + "  ", true);
  }
  else if ((requestType          == SubscribeContext)           &&
           (subscriptionId.get() != "000000000000000000000000") &&
           (subscriptionId.get() != ""))
  {
    out += subscriptionId.render(requestType, indent + "  ", true);
  }

  out += errorCode.render(indent + "  ");

  out += endTag(indent, comma);

  return out;
}



/* ****************************************************************************
*
* check -
*/
std::string SubscribeError::check
(
  RequestType         requestType,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  return "OK";
}
