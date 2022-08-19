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
#include "ngsi/SubscribeResponse.h"



/* ****************************************************************************
*
* SubscribeResponse::SubscribeResponse - 
*/ 
SubscribeResponse::SubscribeResponse()
{
}



/* ****************************************************************************
*
* SubscribeResponse::toJsonV1 -
*/
std::string SubscribeResponse::toJsonV1(bool comma)
{
  std::string  out                 = "";
  std::string  tag                 = "subscribeResponse";
  bool         durationRendered    = !duration.isEmpty();
  bool         throttlingRendered  = !throttling.isEmpty();

  out += startTag(tag);
  out += subscriptionId.toJsonV1(RtSubscribeResponse, durationRendered || throttlingRendered);
  out += duration.toJsonV1(throttlingRendered);
  out += throttling.toJsonV1(false);
  out += endTag(comma);

  return out;
}
