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

#include "ngsi10/UnsubscribeContextResponse.h"
#include "ngsi/StatusCode.h"
#include "ngsi/ErrorCode.h"

#include "unittest.h"



/* ****************************************************************************
*
* constructorsAndRender - 
*/
TEST(UnsubscribeContextResponse, constructorsAndRender)
{
  UnsubscribeContextResponse  uncr1;
  StatusCode                  sc(SccOk, "RP", "D");
  UnsubscribeContextResponse  uncr2(sc);
  ErrorCode                   ec(SccBadRequest, "RP", "D");
  UnsubscribeContextResponse  uncr3(ec);
  std::string                 rendered;
  std::string                 expected = "<unsubscribeContextResponse>\n  <subscriptionId>0</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>RP</reasonPhrase>\n    <details>D</details>\n  </statusCode>\n</unsubscribeContextResponse>\n";

  EXPECT_EQ(0,             uncr1.statusCode.code);
  EXPECT_EQ(SccOk,         uncr2.statusCode.code);
  EXPECT_EQ(SccBadRequest, uncr3.statusCode.code);

  rendered = uncr3.render(UnsubscribeContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

  uncr1.release();
}



/* ****************************************************************************
*
* jsonRender - 
*/
TEST(UnsubscribeContextResponse, jsonRender)
{
  const char*                  filename1  = "ngsi10.unsubscribeContextResponse.jsonRender1.valid.json";
  const char*                  filename2  = "ngsi10.unsubscribeContextResponse.jsonRender2.valid.json";
  UnsubscribeContextResponse*  uncrP;
  std::string                  rendered;

  utInit();

  // Preparations
  uncrP = new UnsubscribeContextResponse();

  // 1. 400, with details
  uncrP->subscriptionId.set("012345678901234567890123");
  uncrP->statusCode.fill(SccBadRequest, "Bad Request", "details");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  rendered = uncrP->render(QueryContext, JSON, "");
  EXPECT_STREQ(expectedBuf, rendered.c_str());



  // 2. 200, no details
  uncrP->subscriptionId.set("012345678901234567890123");
  uncrP->statusCode.fill(SccOk, "OK");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  rendered = uncrP->render(QueryContext, JSON, "");
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  delete uncrP;

  utExit();
}
