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
#include "ngsi10/SubscribeContextResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* constructorsAndRender -
*
*/
TEST(SubscribeContextResponse, constructorsAndRender)
{
  SubscribeContextResponse  scr1;
  StatusCode                ec(SccOk, "D");
  SubscribeContextResponse  scr2(ec);
  std::string               out;

  utInit();

  EXPECT_STREQ("000000000000000000000000", scr1.subscribeError.subscriptionId.get().c_str());
  EXPECT_STREQ("000000000000000000000000", scr2.subscribeError.subscriptionId.get().c_str());
  EXPECT_STREQ("OK", scr2.subscribeError.errorCode.reasonPhrase.c_str());

  utExit();
}



/* ****************************************************************************
*
* jsonRender -
*/
TEST(SubscribeContextResponse, json_render)
{
  const char*                filename1  = "ngsi10.subscribeContextResponse.jsonRender1.valid.json";
  const char*                filename2  = "ngsi10.subscribeContextResponse.jsonRender2.valid.json";
  const char*                filename3  = "ngsi10.subscribeContextResponse.jsonRender3.valid.json";
  const char*                filename4  = "ngsi10.subscribeContextResponse.jsonRender4.valid.json";
  const char*                filename5  = "ngsi10.subscribeContextResponse.jsonRender5.valid.json";
  const char*                filename6  = "ngsi10.subscribeContextResponse.jsonRender6.valid.json";
  SubscribeContextResponse*  scrP;
  std::string                out;

  utInit();

  // Preparations
  scrP = new SubscribeContextResponse();

  // 1. subscribeError, -subscriptionId, with details
  // 2. subscribeError, +subscriptionId, no details
  // 3. subscribeResponse: +subscription -duration -throttling
  // 4. subscribeResponse: +subscription -duration +throttling
  // 5. subscribeResponse: +subscription +duration -throttling
  // 6. subscribeResponse: +subscription +duration +throttling

  // 1.
  scrP->subscribeError.errorCode.fill(SccBadRequest, "details");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  out = scrP->toJsonV1();
  EXPECT_STREQ(expectedBuf, out.c_str());



  // 2.
  scrP->subscribeError.errorCode.fill(SccBadRequest);
  scrP->subscribeError.subscriptionId.set("012345678901234567890123");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  out = scrP->toJsonV1();
  EXPECT_STREQ(expectedBuf, out.c_str());

  scrP->subscribeError.errorCode.fill(SccNone);



  // 3.
  scrP->subscribeResponse.subscriptionId.set("012345678901234567890123");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename3)) << "Error getting test data from '" << filename3 << "'";
  out = scrP->toJsonV1();
  EXPECT_STREQ(expectedBuf, out.c_str());



  // 4.
  scrP->subscribeResponse.throttling.set("PT1M");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename4)) << "Error getting test data from '" << filename4 << "'";
  out = scrP->toJsonV1();
  EXPECT_STREQ(expectedBuf, out.c_str());



  // 5.
  scrP->subscribeResponse.throttling.set("");
  scrP->subscribeResponse.duration.set("PT1H");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename5)) << "Error getting test data from '" << filename5 << "'";
  out = scrP->toJsonV1();
  EXPECT_STREQ(expectedBuf, out.c_str());



  // 6.
  scrP->subscribeResponse.throttling.set("PT1M");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename6)) << "Error getting test data from '" << filename6 << "'";
  out = scrP->toJsonV1();
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
