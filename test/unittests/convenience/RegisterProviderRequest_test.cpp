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

#include "common/globals.h"
#include "convenience/RegisterProviderRequest.h"
#include "jsonParse/jsonRequest.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "jsonParse/jsonRequest.h"
#include "jsonParse/jsonRegisterProviderRequest.h"

#include "testDataFromFile.h"



/* ****************************************************************************
*
* json_ok -
*/
TEST(RegisterProviderRequest, json_ok)
{
  // FIXME P2: gap in outFile
  ParseData       reqData;
  const char*     inFile1  = "ngsi9.registerProviderRequest.noRegistrationId.valid.json";
  const char*     inFile2  = "ngsi9.registerProviderRequest.ok.valid.json";
  const char*     outFile1 = "ngsi9.registerProviderRequestRendered.noRegistrationId.valid.json";  
  const char*     outFile3 = "ngsi9.registerProviderRequest.predetectedError.valid.json";
  const char*     outFile4 = "ngsi9.registerProviderRequestRendered.ok.valid.json";
  std::string     result;
  std::string     rendered;
  std::string     checked;
  ConnectionInfo  ci("", "POST", "1.1");


  // 1. Normal registerProviderRequest
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile1)) << "Error getting test data from '" << inFile1 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile1)) << "Error getting test data from '" << outFile1 << "'";

  result = jsonTreat(testBuf, &ci, &reqData, ContextEntitiesByEntityId, NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";

  rendered = reqData.rpr.res.toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 3. sending a 'predetected error' to the check function
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile3)) << "Error getting test data from '" << outFile3 << "'";

  checked   = reqData.rpr.res.check(V1, DiscoverContextAvailability, "forced predetectedError");
  EXPECT_STREQ(expectedBuf, checked.c_str());


  // 4. Second file
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile2)) << "Error getting test data from '" << inFile2 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile4)) << "Error getting test data from '" << outFile4 << "'";

  result = jsonTreat(testBuf, &ci, &reqData, ContextEntitiesByEntityId, NULL);
  EXPECT_EQ("OK", result);
  rendered = reqData.rpr.res.toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
}
