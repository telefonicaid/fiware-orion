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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/tag.h"
#include "ngsi10/SubscribeContextResponse.h"

/* ****************************************************************************
*
* SubscribeContextResponse::~SubscribeContextResponse -
*/
SubscribeContextResponse::~SubscribeContextResponse() {
    LM_T(LmtDestructor,("destroyed"));
}

/* ****************************************************************************
*
* SubscribeContextResponse::SubscribeContextResponse - 
*/
SubscribeContextResponse::SubscribeContextResponse()
{
   subscribeError.errorCode.keyNameSet("errorCode");
   subscribeError.subscriptionId.set("000000000000000000000000");
}

/* ****************************************************************************
*
* SubscribeContextResponse::SubscribeContextResponse - 
*/
SubscribeContextResponse::SubscribeContextResponse(StatusCode& errorCode)
{
   subscribeError.subscriptionId.set("000000000000000000000000");
   subscribeError.errorCode.fill(&errorCode);
   subscribeError.errorCode.keyNameSet("errorCode");
}

/* ****************************************************************************
*
* SubscribeContextResponse::toJson -
*/
std::string SubscribeContextResponse::toJson(void)
{
  std::string out = "";

  out += "{";

  if (subscribeError.errorCode.code == SccNone)
  {
    // FIXME P5: it is a bit weird to call a toJsonV1() method from a toJson() method. However,
    // SubscribeResponse doesn't have another option. This should be looked into detail.
    out += subscribeResponse.toJsonV1(false);
  }
  else
  {
    out += subscribeError.toJson(SubscribeContext, false);
  }

  out +=  "}";

  return out;
}

/* ****************************************************************************
*
* SubscribeContextResponse::toJsonV1 -
*/
std::string SubscribeContextResponse::toJsonV1(void)
{
  std::string out     = "";

  out += startTag();

  if (subscribeError.errorCode.code == SccNone)
  {
    out += subscribeResponse.toJsonV1(false);
  }
  else
  {
    out += subscribeError.toJsonV1(SubscribeContext, false);
  }

  out += endTag(false);

  return out;
}
