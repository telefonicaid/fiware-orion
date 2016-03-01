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

#include "testDataFromFile.h"
#include "common/globals.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "jsonParse/jsonRequest.h"

#include "ngsi/SubscriptionId.h"
#include "ngsi9/UnsubscribeContextAvailabilityRequest.h"

#include "unittest.h"



/* ****************************************************************************
*
* constructorsAndCheck -
*
* FIXME P5 #1862: _json countepart?
*/
TEST(UnsubscribeContextAvailabilityRequest, DISABLED_constructorAndCheck)
{
  UnsubscribeContextAvailabilityRequest ucar1;
  SubscriptionId                        subId("012345678901234567890123");
  UnsubscribeContextAvailabilityRequest ucar2(subId);

  utInit();

  EXPECT_EQ("", ucar1.subscriptionId.get());
  EXPECT_EQ("012345678901234567890123", ucar2.subscriptionId.get());

  std::string   out;
  const char*   outfile1 = "ngsi9.unsubscribeContextAvailabilityResponse.forcedError.valid.xml";
  const char*   outfile2 = "ngsi9.unsubscribeContextAvailabilityResponse.invalidSubscriptionId.valid.xml";

  out = ucar1.check(UnsubscribeContextAvailability, "", "Forced Error", 0);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucar1.subscriptionId.set("1");
  out = ucar1.check(UnsubscribeContextAvailability, "", "", 0);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = ucar2.check(UnsubscribeContextAvailability, "", "", 0);
  EXPECT_EQ("OK", out);

  utExit();
}



/* ****************************************************************************
*
* badSubscriptionId_json - 
*/
TEST(UnsubscribeContextAvailabilityRequest, badSubscriptionId_json)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi9.unsubscribeContextAvailabilityRequest.badSubscriptionId.invalid.json";
  const char*     outfile = "ngsi9.unsubscribeContextAvailabilityResponse.badSubscriptionId.valid.json";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  
  ci.inFormat  = JSON;
  ci.outFormat = JSON;
  lmTraceLevelSet(LmtDump, true);
  std::string out = jsonTreat(testBuf, &ci, &reqData, UnsubscribeContextAvailability, "unsubscribeContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  lmTraceLevelSet(LmtDump, false);

  UnsubscribeContextAvailabilityRequest*  ucarP = &reqData.ucar.res;

  ucarP->release();

  utExit();
}
