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

#include "common/globals.h"
#include "jsonParse/jsonRequest.h"
#include "ngsi/ParseData.h"

#include "unittest.h"



/* ****************************************************************************
*
* Tests
*   xml_ok
*   json_ok
*   xml_badIsPattern
*   json_badIsPattern
*   xml_noEntityId
*   json_noEntityId
*   xml_badEntityId

*   xml_entityIdTypeAsBothFieldAndAttribute
*   xml_entityIdIsPatternAsBothFieldAndAttribute
*   xml:scrPresent
*   json_badDuration
*   jsonScarRelease
*   jsonScarPresent
*
*/



/* ****************************************************************************
*
* json_ok -
*/
TEST(SubscribeContextAvailabilityRequest, json_ok)
{
  ParseData       reqData;
  const char*     infile = "ngsi9.subscribeContextAvailabilityRequest.ok.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  utInit();

  ci.inMimeType      = JSON;
  ci.outMimeType     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ("OK", result) << "this test should be OK";

  utExit();
}



/* ****************************************************************************
*
* json_badIsPattern -
*/
TEST(SubscribeContextAvailabilityRequest, json_badIsPattern)
{
  ParseData       reqData;
  const char*     infile = "ngsi9.subscribeContextAvailabilityRequest.badIsPattern.invalid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  utInit();

  ci.inMimeType      = JSON;
  ci.outMimeType     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result);

  utExit();
}



/* ****************************************************************************
*
* json_noEntityId -
*/
TEST(SubscribeContextAvailabilityRequest, json_noEntityId)
{
  ParseData       reqData;
  const char*     infile  = "ngsi9.subscribeContextAvailabilityRequest.noEntityId.invalid.json";
  const char*     outfile = "ngsi9.subscribeContextAvailabilityResponse.noEntityId.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  utInit();

  ci.inMimeType      = JSON;
  ci.outMimeType     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* json_badDuration -
*/
TEST(SubscribeContextAvailabilityRequest, json_badDuration)
{
  ParseData       reqData;
  const char*     infile  = "ngsi9.subscribeContextAvailabilityRequest.badDuration.invalid.json";
  const char*     outfile = "ngsi9.subscribeContextAvailabilityResponse.badDuration.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  utInit();

  ci.inMimeType      = JSON;
  ci.outMimeType     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
