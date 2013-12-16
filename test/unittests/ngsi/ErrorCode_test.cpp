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

#include "ngsi/ErrorCode.h"



/* ****************************************************************************
*
* render - 
*/
TEST(ErrorCode, render)
{
  ErrorCode    e1;
  ErrorCode    e2(200, "REASON", "DETAILS");
  std::string  rendered;
  std::string  expected1xml  = "<errorCode>\n  <code>500</code>\n  <reasonPhrase> - ZERO code set to 500</reasonPhrase>\n</errorCode>\n";
  std::string  expected1json = "\"errorCode\" : {\n  \"code\" : \"500\",\n  \"reasonPhrase\" : \" - ZERO code set to 500\"\n}\n";
  std::string  expected2xml  = "<errorCode>\n  <code>200</code>\n  <reasonPhrase>REASON</reasonPhrase>\n  <details>DETAILS</details>\n</errorCode>\n";

  rendered = e1.render(XML, "");
  EXPECT_STREQ(expected1xml.c_str(), rendered.c_str());
  rendered = e1.render(JSON, "");
  EXPECT_STREQ(expected1json.c_str(), rendered.c_str());

  rendered = e2.render(XML, "");
  EXPECT_STREQ(expected2xml.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* check - 
*/
TEST(ErrorCode, check)
{
  ErrorCode    e1(0, "REASON", "DETAILS");
  ErrorCode    e2(200, "", "DETAILS");
  ErrorCode    e3(200, "REASON", "DETAILS");
  std::string  rendered;
  std::string  expected1 = "no code";
  std::string  expected2 = "no reason phrase";
  std::string  expected3 = "OK";

  rendered = e1.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  rendered = e2.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = e3.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());
}
