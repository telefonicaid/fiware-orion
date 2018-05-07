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
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi9/RegisterContextResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* constructors -
*
*/
TEST(RegisterContextResponse, constructors)
{
  RegisterContextResponse* rcr1 = new RegisterContextResponse();
  RegisterContextResponse  rcr2("012301230123012301230123", "PT1S");
  RegisterContextRequest   rcr;
  RegisterContextResponse  rcr3(&rcr);
  StatusCode               ec(SccBadRequest, "Detail");
  RegisterContextResponse  rcr4("012345678901234567890123", ec);
  RegisterContextResponse  rcr5("012345678901234567890123", "PT1M");

  std::string              out;

  std::string              expected5 = "OK";

  EXPECT_STREQ("", rcr1->registrationId.get().c_str());
  rcr1->release();
  delete rcr1;

  EXPECT_EQ("012301230123012301230123", rcr2.registrationId.get());
  EXPECT_STREQ("", rcr3.registrationId.get().c_str());
  EXPECT_EQ("012345678901234567890123", rcr4.registrationId.get());
  EXPECT_EQ(SccBadRequest, rcr4.errorCode.code);
    
  out = rcr5.check("", 0);

  EXPECT_EQ(expected5, out);

  rcr2.present("");
}



/* ****************************************************************************
*
* jsonRender -
*/
TEST(RegisterContextResponse, jsonRender)
{
  RegisterContextResponse rcr;
  std::string             rendered;
  const char*             filename1 = "ngsi9.registerContextResponse.registrationIdOnly.valid.json";
  const char*             filename2 = "ngsi9.registerContextResponse.registrationIdAndDuration.valid.json";
  const char*             filename3 = "ngsi9.registerContextResponse.registrationIdAndErrorCode.valid.json";
  const char*             filename4 = "ngsi9.registerContextResponse.registrationIdAndDurationAndErrorCode.valid.json";

  utInit();

  // 1. Only registrationId
  rcr.registrationId.set("012345678901234567890123");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  rendered = rcr.render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  // 2. registrationId and duration
  rcr.duration.set("PT1S");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  rendered = rcr.render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  // 3. registrationId and errorCode
  rcr.duration.set("");
  rcr.errorCode.fill(SccBadRequest, "no details");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename3)) << "Error getting test data from '" << filename3 << "'";
  rendered = rcr.render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  // 4. registrationId and duration and errorCode
  rcr.duration.set("PT2S");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename4)) << "Error getting test data from '" << filename4 << "'";
  rendered = rcr.render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  utExit();
}
