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
#include "ngsi10/UnsubscribeContextResponse.h"
#include "ngsi10/UnsubscribeContextRequest.h"



/* ****************************************************************************
*
* UnsubscribeContextRequest::render - 
*/
std::string UnsubscribeContextRequest::render(const std::string& indent)
{
  std::string out = "";

  out += startTag(indent);
  out += subscriptionId.render(UnsubscribeContext, indent + "  ");
  out += endTag(indent);

  return out;
}



/* ****************************************************************************
*
* UnsubscribeContextRequest::check - 
*/
std::string UnsubscribeContextRequest::check(const std::string& indent, const std::string& predetectedError, int counter)
{
  UnsubscribeContextResponse  response;
  std::string                 res;

  if ((res = subscriptionId.check(SubscribeContext, indent, predetectedError, counter)) != "OK")
  {
     response.statusCode.fill(SccBadRequest, std::string("Invalid Subscription Id: /") + subscriptionId.get() + "/: " + res);
     return response.render(indent);
  }

  return "OK";
}



/* ****************************************************************************
*
* UnsubscribeContextRequest::present - 
*/
void UnsubscribeContextRequest::present(const std::string& indent)
{
   subscriptionId.present(indent);
}



/* ****************************************************************************
*
* UnsubscribeContextRequest::release - 
*/
void UnsubscribeContextRequest::release(void)
{
}
