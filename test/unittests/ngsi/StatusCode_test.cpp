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

#include "ngsi/StatusCode.h"
#include "ngsi/ErrorCode.h"



/* ****************************************************************************
*
* render - 
*/
TEST(StatusCode, render)
{
  StatusCode    sc1;
  StatusCode    sc2(SccOk, "REASON", "");
  StatusCode    sc3(SccOk, "REASON", "DETAILS");
  StatusCode    sc4(SccOk, "REASON", "DETAILS");
  std::string   rendered;
  std::string   expected1 = "<statusCode>\n  <code>0</code>\n  <reasonPhrase></reasonPhrase>\n</statusCode>\n";
  std::string   expected2 = "<statusCode>\n  <code>200</code>\n  <reasonPhrase>REASON</reasonPhrase>\n</statusCode>\n";
  std::string   expected3 = "<statusCode>\n  <code>200</code>\n  <reasonPhrase>REASON</reasonPhrase>\n  <details>DETAILS</details>\n</statusCode>\n";
  std::string   expected4 = "\"statusCode\" : {\n  \"code\" : \"200\",\n  \"reasonPhrase\" : \"REASON\",\n  \"details\" : \"DETAILS\"\n}\n";

  rendered = sc1.render(XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  rendered = sc2.render(XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = sc3.render(XML, "");
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());

  rendered = sc4.render(JSON, "");
  EXPECT_STREQ(expected4.c_str(), rendered.c_str());

  sc1.release(); // just to exercise the code ...
}



/* ****************************************************************************
*
* fill - 
*/
TEST(StatusCode, fill)
{
  StatusCode    sc;
  StatusCode    sc2(SccOk, "Reason", "Details");
  ErrorCode     ec(SccBadRequest, "Bad request", "Very bad request :-)");
  std::string   rendered;
  std::string   expected1 = "<statusCode>\n  <code>200</code>\n  <reasonPhrase></reasonPhrase>\n</statusCode>\n";

  sc.fill(SccForbidden, "R", "D");
  EXPECT_EQ(sc.code, SccForbidden);
  EXPECT_STREQ(sc.reasonPhrase.c_str(), "R");
  EXPECT_STREQ(sc.details.c_str(), "D");

  sc.fill(&sc2);
  EXPECT_EQ(sc.code, SccOk);
  EXPECT_STREQ(sc.reasonPhrase.c_str(), "Reason");
  EXPECT_STREQ(sc.details.c_str(), "Details");

  sc.fill(&ec);
  EXPECT_EQ(sc.code, SccBadRequest);
  EXPECT_STREQ(sc.reasonPhrase.c_str(), "Bad request");
  EXPECT_STREQ(sc.details.c_str(), "Very bad request :-)");
}



/* ****************************************************************************
*
* check - 
*/
TEST(StatusCode, check)
{
  StatusCode    sc(SccOk, "REASON", "");
  std::string   rendered;
  std::string   expected1 = "OK";
  std::string   expected2 = "no code";
  std::string   expected3 = "no reason phrase";

  rendered = sc.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  sc.fill((HttpStatusCode) 0, "XXX", "YYY");
  rendered = sc.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  sc.fill(SccOk, "", "YYY");
  rendered = sc.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(StatusCode, present)
{
  StatusCode    sc(SccOk, "REASON", "");
  std::string   rendered;
  std::string   expected1 = "<statusCode>\n  <code>200</code>\n  <reasonPhrase></reasonPhrase>\n</statusCode>\n";

  sc.present("");
}



