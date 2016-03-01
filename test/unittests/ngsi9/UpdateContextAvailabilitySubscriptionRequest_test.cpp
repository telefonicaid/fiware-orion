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
#include "rest/ConnectionInfo.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* xml_invalidEntityAttribute - 
*
* FIXME P5 #1862: _json counterpart?
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, DISABLED_xml_invalidEntityAttribute)
{
#if 0
  ParseData       reqData;
  const char*     infile  = "ngsi9.updateContextAvailabilitySubscriptionRequest.entityIdAttribute.invalid.xml";
  const char*     outfile = "ngsi9.updateContextAvailabilitySubscriptionResponse.entityIdAttribute.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string out = xmlTreat(testBuf, &ci, &reqData, UpdateContextAvailabilitySubscription, "updateContextAvailabilitySubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
#endif
}



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

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf,     sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  lmTraceLevelSet(LmtDump, true);
  out = jsonTreat(testBuf, &ci, &parseData, UpdateContextAvailabilitySubscription, "updateContextAvailabilitySubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ("OK", out) << "this test should be OK";

  UpdateContextAvailabilitySubscriptionRequest* ucasP = &parseData.ucas.res;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  out = ucasP->render(UpdateContextAvailabilitySubscription, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  out = ucasP->check(&ci, UpdateContextAvailabilitySubscription, "", "predetected error", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());
  
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  ucasP->duration.set("eeeee");
  out = ucasP->check(&ci, UpdateContextAvailabilitySubscription, "", "", 0);
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

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = jsonTreat(testBuf, &ci, &reqData, UpdateContextAvailabilitySubscription, "updateContextAvailabilitySubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* response - 
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, response)
{
  UpdateContextAvailabilitySubscriptionResponse  ucas;
  StatusCode                                     ec(SccBadRequest, "Detail");
  UpdateContextAvailabilitySubscriptionResponse  ucas2(ec);
  std::string                                    out;
  const char*                                    outfile1 = "ngsi9.updateContextAvailabilitySubscriptionResponse.response1.valid.xml";
  const char*                                    outfile2 = "ngsi9.updateContextAvailabilitySubscriptionResponse.response2.valid.xml";
  const char*                                    outfile3 = "ngsi9.updateContextAvailabilitySubscriptionResponse.response3.valid.xml";
  const char*                                    outfile4 = "ngsi9.updateContextAvailabilitySubscriptionResponse.response4.valid.xml";
  
  utInit();

  EXPECT_EQ(ucas2.errorCode.code, SccBadRequest);

  ucas.subscriptionId.set("012345678901234567890123");

  out = ucas.check(UpdateContextAvailabilitySubscription, "", "", 0);
  EXPECT_EQ("OK", out);
  
  out = ucas.render(UpdateContextAvailabilitySubscription, "", 0);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucas.errorCode.fill(SccBadRequest, "Detail");
  out = ucas.render(UpdateContextAvailabilitySubscription, "", 0);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());
  
  ucas.errorCode.fill(SccNone);
  ucas.duration.set("ddd");
  out = ucas.check(UpdateContextAvailabilitySubscription, "", "", 0);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = ucas.check(UpdateContextAvailabilitySubscription, "", "predetected error", 0);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile4)) << "Error getting test data from '" << outfile4 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
