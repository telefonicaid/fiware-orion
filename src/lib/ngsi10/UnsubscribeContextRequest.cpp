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
* UnsubscribeContextRequest::toJsonV1 -
*/
std::string UnsubscribeContextRequest::toJsonV1(void)
{
  std::string out = "";

  out += startTag();
  out += subscriptionId.toJsonV1(UnsubscribeContext, false);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* UnsubscribeContextRequest::check - 
*/
std::string UnsubscribeContextRequest::check()
{
  UnsubscribeContextResponse  response;
  std::string                 res;

  if ((res = subscriptionId.check()) != "OK")
  {
     response.statusCode.fill(SccBadRequest, std::string("Invalid Subscription Id: /") + subscriptionId.get() + "/: " + res);
     return response.toJsonV1();
  }

  return "OK";
}



/* ****************************************************************************
*
* UnsubscribeContextRequest::release - 
*/
void UnsubscribeContextRequest::release(void)
{
}
