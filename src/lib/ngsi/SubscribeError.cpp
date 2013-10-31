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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>

#include "common/Format.h"
#include "common/tag.h"
#include "ngsi/ErrorCode.h"
#include "ngsi/Request.h"
#include "ngsi/SubscribeError.h"



/* ****************************************************************************
*
* SubscribeError::SubscribeError - 
*/
SubscribeError::SubscribeError()
{
}



/* ****************************************************************************
*
* SubscribeError::render - 
*/
std::string SubscribeError::render(RequestType requestType, Format format, std::string indent)
{
  std::string out = "";
  std::string tag = "subscribeError";

  out += startTag(indent, tag, format, true);

  // subscriptionId is Mandatory if part of updateContextSubscriptionResponse
  if (requestType == UpdateContextSubscription)
     out += subscriptionId.render(format, indent + "  ", true);
  else if ((requestType == SubscribeContext) && (subscriptionId.get() != "0") && (subscriptionId.get() != ""))
     out += subscriptionId.render(format, indent + "  ", true);

  out += errorCode.render(format, indent + "  ");

  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string SubscribeError::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
   return "OK";
}
