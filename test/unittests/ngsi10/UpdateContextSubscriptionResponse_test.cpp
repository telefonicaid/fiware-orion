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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi10/UpdateContextSubscriptionResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* jsonRender -
*/
TEST(UpdateContextSubscriptionResponse, json_render)
{
  const char*                         filename1  = "ngsi10.updateContextSubscriptionResponse.jsonRender1.valid.json";
  const char*                         filename2  = "ngsi10.updateContextSubscriptionResponse.jsonRender2.valid.json";
  const char*                         filename3  = "ngsi10.updateContextSubscriptionResponse.jsonRender3.valid.json";
  const char*                         filename4  = "ngsi10.updateContextSubscriptionResponse.jsonRender4.valid.json";
  const char*                         filename5  = "ngsi10.updateContextSubscriptionResponse.jsonRender5.valid.json";
  const char*                         filename6  = "ngsi10.updateContextSubscriptionResponse.jsonRender6.valid.json";
  UpdateContextSubscriptionResponse*  ucsrP;
  std::string                         out;

  utInit();

  // Preparations
  ucsrP = new UpdateContextSubscriptionResponse();

  // 1. subscribeError, -subscriptionId, with details
  // 2. subscribeError, +subscriptionId, no details
  // 3. subscribeResponse: +subscription -duration -throttling
  // 4. subscribeResponse: +subscription -duration +throttling
  // 5. subscribeResponse: +subscription +duration -throttling
  // 6. subscribeResponse: +subscription +duration +throttling

  // 1.
  ucsrP->subscribeError.errorCode.fill(SccBadRequest, "details");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  out = ucsrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());



  // 2.
  ucsrP->subscribeError.errorCode.fill(SccBadRequest);
  ucsrP->subscribeError.subscriptionId.set("012345678901234567890123");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  out = ucsrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucsrP->subscribeError.errorCode.fill(SccNone);



  // 3.
  ucsrP->subscribeResponse.subscriptionId.set("012345678901234567890123");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename3)) << "Error getting test data from '" << filename3 << "'";
  out = ucsrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());



  // 4.
  ucsrP->subscribeResponse.throttling.set("PT1M");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename4)) << "Error getting test data from '" << filename4 << "'";
  out = ucsrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());



  // 5.
  ucsrP->subscribeResponse.throttling.set("");
  ucsrP->subscribeResponse.duration.set("PT1H");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename5)) << "Error getting test data from '" << filename5 << "'";
  out = ucsrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());



  // 6.
  ucsrP->subscribeResponse.throttling.set("PT1M");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename6)) << "Error getting test data from '" << filename6 << "'";
  out = ucsrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
