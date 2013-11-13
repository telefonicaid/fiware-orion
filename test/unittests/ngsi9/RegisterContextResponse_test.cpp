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

#include "ngsi9/RegisterContextResponse.h"



/* ****************************************************************************
*
* constructors - 
*/
TEST(RegisterContextResponse, constructors)
{
  RegisterContextResponse* rcr1 = new RegisterContextResponse();
  RegisterContextResponse  rcr2("REG_ID2", "PT1S");
  RegisterContextRequest   rcr;
  RegisterContextResponse  rcr3(&rcr);
  ErrorCode                ec(SccBadRequest, "Reason", "Detail");
  RegisterContextResponse  rcr4("012345678901234567890123", ec);
  RegisterContextResponse  rcr5("012345678901234567890123", "PT1M");

  std::string              out;
  std::string              expected1 = "<registerContextResponse>\n  <duration>PT1S</duration>\n  <registrationId>REG_ID2</registrationId>\n</registerContextResponse>\n";
  std::string              expected2 = "<registerContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad length (24 chars expected)</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  std::string              expected3 = "<registerContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Forced Error</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  std::string              expected4 = "<registerContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>syntax error in duration string</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  std::string              expected5 = "OK";

  EXPECT_STREQ("", rcr1->registrationId.get().c_str());
  rcr1->release();
  delete rcr1;

  EXPECT_EQ("REG_ID2", rcr2.registrationId.get());
  EXPECT_STREQ("", rcr3.registrationId.get().c_str());
  EXPECT_EQ("012345678901234567890123", rcr4.registrationId.get());
  EXPECT_EQ(SccBadRequest, rcr4.errorCode.code);
  
  out = rcr2.render(RegisterContext, XML, "");
  EXPECT_EQ(expected1, out);

  out = rcr2.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ(expected2, out);

  out = rcr2.check(RegisterContext, XML, "", "Forced Error", 0);
  EXPECT_EQ(expected3, out);

  rcr2.duration.set("dddd");
  out = rcr2.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ(expected4, out);
  
  out = rcr5.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ(expected5, out);

  rcr2.present("");
}
