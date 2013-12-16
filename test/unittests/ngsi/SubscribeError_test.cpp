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
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/SubscribeError.h"



/* ****************************************************************************
*
* render - 
*/
TEST(SubscribeError, render)
{
  SubscribeError  se;
  std::string     rendered;
  std::string     expected1xml  = "<subscribeError>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>reason</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</subscribeError>\n";
  std::string     expected2xml  = "<subscribeError>\n  <subscriptionId>SUB_123</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>reason</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</subscribeError>\n";
  std::string     expected1json = "\"subscribeError\" : {\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"reason\",\n    \"details\" : \"detail\"\n  }\n}\n";

  se.subscriptionId.set("SUB_123");
  se.errorCode.fill(SccBadRequest, "reason", "detail");

  rendered = se.render(RegisterContext, XML, "");
  EXPECT_STREQ(expected1xml.c_str(), rendered.c_str());
  rendered = se.render(RegisterContext, JSON, "");
  EXPECT_STREQ(expected1json.c_str(), rendered.c_str());

  rendered = se.render(SubscribeContext, XML, "");
  EXPECT_STREQ(expected2xml.c_str(), rendered.c_str());
}


/* ****************************************************************************
*
* check - 
*/
TEST(SubscribeError, check)
{
  SubscribeError  se;
  std::string     checked;
  std::string     expected = "OK";

  checked = se.check(SubscribeContext, XML, "", "", 0);
  EXPECT_STREQ(expected.c_str(), checked.c_str());
}
