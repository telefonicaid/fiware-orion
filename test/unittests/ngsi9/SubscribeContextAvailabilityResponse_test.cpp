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

#include "ngsi9/SubscribeContextAvailabilityResponse.h"
#include "unittest.h"



/* ****************************************************************************
*
* constructors -
*/
TEST(SubscribeContextAvailabilityResponse, constructors)
{
  SubscribeContextAvailabilityResponse* scar1 = new SubscribeContextAvailabilityResponse();
  SubscribeContextAvailabilityResponse  scar2("012345678901234567890123", "PT1S");
  StatusCode                            ec(SccBadRequest, "Detail");
  SubscribeContextAvailabilityResponse  scar3("012345678901234567890124", ec);

  utInit();

  EXPECT_EQ("", scar1->subscriptionId.get());
  delete(scar1);

  EXPECT_EQ("012345678901234567890123", scar2.subscriptionId.get());

  EXPECT_EQ("012345678901234567890124", scar3.subscriptionId.get());
  EXPECT_EQ(SccBadRequest, scar3.errorCode.code);

  utExit();
}



/* ****************************************************************************
*
* jsonRender -
*
* subscriptionId: MANDATORY
* duration:       Optional
* errorCode:      Optional
*/
TEST(SubscribeContextAvailabilityResponse, jsonRender)
{
  const char*                            filename1  = "ngsi9.subscribeContextAvailabilityResponse.jsonRender1.valid.json";
  const char*                            filename2  = "ngsi9.subscribeContextAvailabilityResponse.jsonRender2.valid.json";
  const char*                            filename3  = "ngsi9.subscribeContextAvailabilityResponse.jsonRender3.valid.json";
  const char*                            filename4  = "ngsi9.subscribeContextAvailabilityResponse.jsonRender4.valid.json";
  SubscribeContextAvailabilityResponse*  scarP;
  std::string                            rendered;
  utInit();

  // Preparations
  scarP = new SubscribeContextAvailabilityResponse();

  // 1. +subscriptionId -duration -errorCode
  scarP->subscriptionId.set("012345678901234567890123");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  rendered = scarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 2. +subscriptionId -duration +errorCode
  scarP->errorCode.fill(SccBadRequest);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  rendered = scarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 3. +subscriptionId +duration -errorCode
  scarP->errorCode.fill(SccNone);
  scarP->duration.set("PT1H");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename3)) << "Error getting test data from '" << filename3 << "'";
  rendered = scarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 4. +subscriptionId +duration +errorCode
  scarP->errorCode.fill(SccBadRequest, "no details");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename4)) << "Error getting test data from '" << filename4 << "'";
  rendered = scarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  free(scarP);

  utExit();
}
