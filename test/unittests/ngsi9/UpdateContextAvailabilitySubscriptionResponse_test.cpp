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

#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"
#include "unittest.h"



/* ****************************************************************************
*
* jsonRender -
*
* subscriptionId: MANDATORY
* duration:       Optional
* errorCode:      Optional
*/
TEST(UpdateContextAvailabilitySubscriptionResponse, jsonRender)
{
  const char*                                     filename1  = "ngsi9.updateContextAvailabilitySubscriptionResponse.jsonRender1.valid.json";
  const char*                                     filename2  = "ngsi9.updateContextAvailabilitySubscriptionResponse.jsonRender2.valid.json";
  const char*                                     filename3  = "ngsi9.updateContextAvailabilitySubscriptionResponse.jsonRender3.valid.json";
  const char*                                     filename4  = "ngsi9.updateContextAvailabilitySubscriptionResponse.jsonRender4.valid.json";
  UpdateContextAvailabilitySubscriptionResponse*  ucasP;
  std::string                                     rendered;

  utInit();

  // Preparations
  ucasP = new UpdateContextAvailabilitySubscriptionResponse();

  // 1. +subscriptionId -duration -errorCode
  ucasP->subscriptionId.set("012345678901234567890123");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  rendered = ucasP->render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 2. +subscriptionId -duration +errorCode
  ucasP->errorCode.fill(SccBadRequest);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  rendered = ucasP->render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 3. +subscriptionId +duration -errorCode
  ucasP->errorCode.fill(SccNone);
  ucasP->duration.set("PT1H");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename3)) << "Error getting test data from '" << filename3 << "'";
  rendered = ucasP->render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 4. +subscriptionId +duration +errorCode
  ucasP->errorCode.fill(SccBadRequest, "no details");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename4)) << "Error getting test data from '" << filename4 << "'";
  rendered = ucasP->render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  free(ucasP);

  utExit();
}
