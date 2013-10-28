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

#include "ngsi10/SubscribeContextResponse.h"



/* ****************************************************************************
*
* constructorsAndRender - 
*/
TEST(SubscribeContextResponse, constructorsAndRender)
{
  SubscribeContextResponse  scr1;
  ErrorCode                 ec(SccOk, "RP", "D");
  SubscribeContextResponse  scr2(ec);
  std::string               rendered;
  std::string               expected = "<subscribeContextResponse>\n  <subscribeError>\n    <errorCode>\n      <code>200</code>\n      <reasonPhrase>RP</reasonPhrase>\n      <details>D</details>\n    </errorCode>\n  </subscribeError>\n</subscribeContextResponse>\n";

  EXPECT_STREQ("0", scr1.subscribeError.subscriptionId.get().c_str());
  EXPECT_STREQ("0", scr2.subscribeError.subscriptionId.get().c_str());
  EXPECT_STREQ("RP", scr2.subscribeError.errorCode.reasonPhrase.c_str());

  rendered = scr2.render(SubscribeContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());
}
