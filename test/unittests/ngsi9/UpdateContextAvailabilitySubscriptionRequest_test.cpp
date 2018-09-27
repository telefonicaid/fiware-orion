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
#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* json_ok -
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, json_ok)
{
  ConnectionInfo  ci("", "POST", "1.1");
  ParseData       parseData;
  const char*     infile    = "ngsi9.updateContextAvailabilitySubscriptionRequest.ok.valid.json";
  const char*     outfile1  = "ngsi9.updateContextAvailabilitySubscriptionRequest.expected1.valid.json";
  const char*     outfile2  = "ngsi9.updateContextAvailabilitySubscriptionRequest.expected2.valid.json";
  const char*     outfile3  = "ngsi9.updateContextAvailabilitySubscriptionRequest.expected3.valid.json";
  std::string     out;

  utInit();

  ci.inMimeType      = JSON;
  ci.outMimeType     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf,     sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  lmTraceLevelSet(LmtDump, true);
  out = jsonTreat(testBuf, &ci, &parseData, UpdateContextAvailabilitySubscription, NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ("OK", out) << "this test should be OK";

  UpdateContextAvailabilitySubscriptionRequest* ucasP = &parseData.ucas.res;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  out = ucasP->toJsonV1();
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  out = ucasP->check("predetected error", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  ucasP->duration.set("eeeee");
  out = ucasP->check("", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* json_invalidIsPattern -
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, json_invalidIsPattern)
{
  ParseData       reqData;
  const char*     infile  = "updateContextAvailabilitySubscriptionRequest_invalidIsPattern.json";
  const char*     outfile = "ngsi9.updateContextAvailabilitySubscriptionResponse.invalidIsPattern.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  utInit();

  ci.inMimeType      = JSON;
  ci.outMimeType     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = jsonTreat(testBuf, &ci, &reqData, UpdateContextAvailabilitySubscription, NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* response -
*
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, response)
{
  UpdateContextAvailabilitySubscriptionResponse  ucas;
  StatusCode                                     ec(SccBadRequest, "Detail");
  UpdateContextAvailabilitySubscriptionResponse  ucas2(ec);
  std::string                                    out;

  utInit();

  EXPECT_EQ(ucas2.errorCode.code, SccBadRequest);

  ucas.subscriptionId.set("012345678901234567890123");

  out = ucas.check("");
  EXPECT_EQ("OK", out);

  utExit();
}
