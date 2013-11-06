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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "jsonParse/jsonRequest.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/xmlRequest.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"

#include "testDataFromFile.h"



/* ****************************************************************************
*
* xml_ok - 
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, xml_ok)
{
  ParseData       reqData;
  const char*     fileName = "updateContextAvailabilitySubscriptionRequest_ok.xml";
  ConnectionInfo  ci("", "POST", "1.1");  
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContextAvailabilitySubscription, "updateContextAvailabilitySubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ("OK", result) << "this test should be OK";
}



/* ****************************************************************************
*
* xml_invalidEntityAttribute - 
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, xml_invalidEntityAttribute)
{
  ParseData       reqData;
  const char*     fileName = "updateContextAvailabilitySubscriptionRequest_invalidEntityAttribute.xml";
  ConnectionInfo  ci("", "POST", "1.1");  
  std::string     expected = "<updateContextAvailabilitySubscriptionResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n</updateContextAvailabilitySubscriptionResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContextAvailabilitySubscription, "updateContextAvailabilitySubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* json_ok - 
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, json_ok)
{
  ParseData       reqData;
  const char*     fileName = "updateContextAvailabilitySubscriptionRequest_ok.json";
  ConnectionInfo  ci("", "POST", "1.1");
  std::string     expected1 =   "\"updateContextAvailabilitySubscriptionRequest\" : {\n  \"entities\" : [\n    {\n      \"type\" : \"Room\",\n      \"isPattern\" : \"false\",\n      \"id\" : \"ConferenceRoom\"\n    },\n    {\n      \"type\" : \"Room\",\n      \"isPattern\" : \"false\",\n      \"id\" : \"OfficeRoom\"\n    }\n  ]\n  \"attributeList\" : {\n    \"attribute\" : \"temperature\",\n    \"attribute\" : \"occupancy\",\n    \"attribute\" : \"lightstatus\"\n  }\n  \"duration\" : \"PT1M\",\n  \"restriction\" : {\n    \"attributeExpression\" : \"AE\"\n    \"scope\" : {\n      \"operationScope\" : {\n        \"type\" : \"st1\"\n        \"value\" : \"1\"\n      }\n      \"operationScope\" : {\n        \"type\" : \"st2\"\n        \"value\" : \"2\"\n      }\n    }\n  }\n  \"subscriptionId\" : \"012345678901234567890123\"\n}\n";
  std::string     expected2 = "\"updateContextAvailabilitySubscriptionRequest\" : {\n  \"entities\" : [\n    {\n      \"type\" : \"Room\",\n      \"isPattern\" : \"false\",\n      \"id\" : \"ConferenceRoom\"\n    },\n    {\n      \"type\" : \"Room\",\n      \"isPattern\" : \"false\",\n      \"id\" : \"OfficeRoom\"\n    }\n  ]\n  \"attributeList\" : {\n    \"attribute\" : \"temperature\",\n    \"attribute\" : \"occupancy\",\n    \"attribute\" : \"lightstatus\"\n  }\n  \"duration\" : \"PT1M\",\n  \"restriction\" : {\n    \"attributeExpression\" : \"AE\"\n    \"scope\" : {\n      \"operationScope\" : {\n        \"type\" : \"st1\"\n        \"value\" : \"1\"\n      }\n      \"operationScope\" : {\n        \"type\" : \"st2\"\n        \"value\" : \"2\"\n      }\n    }\n  }\n  \"subscriptionId\" : \"012345678901234567890123\"\n}\n";
  std::string     expected3 = "\"updateContextAvailabilitySubscriptionRequest\" : {\n  \"entities\" : [\n    {\n      \"type\" : \"Room\",\n      \"isPattern\" : \"false\",\n      \"id\" : \"ConferenceRoom\"\n    },\n    {\n      \"type\" : \"Room\",\n      \"isPattern\" : \"false\",\n      \"id\" : \"OfficeRoom\"\n    }\n  ]\n  \"attributeList\" : {\n    \"attribute\" : \"temperature\",\n    \"attribute\" : \"occupancy\",\n    \"attribute\" : \"lightstatus\"\n  }\n  \"duration\" : \"PT1M\",\n  \"restriction\" : {\n    \"attributeExpression\" : \"AE\"\n    \"scope\" : {\n      \"operationScope\" : {\n        \"type\" : \"st1\"\n        \"value\" : \"1\"\n      }\n      \"operationScope\" : {\n        \"type\" : \"st2\"\n        \"value\" : \"2\"\n      }\n    }\n  }\n  \"subscriptionId\" : \"012345678901234567890123\"\n}\n";
  std::string     rendered;
  std::string     check;

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, UpdateContextAvailabilitySubscription, "updateContextAvailabilitySubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ("OK", result) << "this test should be OK";

  UpdateContextAvailabilitySubscriptionRequest* ucasP = &reqData.ucas.res;

  rendered = ucasP->render(UpdateContextAvailabilitySubscription, JSON, "");
  EXPECT_EQ(expected1, rendered);

  check = ucasP->check(UpdateContextAvailabilitySubscription, JSON, "", "predetected error", 0);
  EXPECT_EQ(expected2, rendered);
  
  ucasP->duration.set("eeeee");
  check = ucasP->check(UpdateContextAvailabilitySubscription, JSON, "", "", 0);
  EXPECT_EQ(expected3, rendered);
}



/* ****************************************************************************
*
* json_ok - 
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, json_invalidIsPattern)
{
  ParseData       reqData;
  const char*     fileName = "updateContextAvailabilitySubscriptionRequest_invalidIsPattern.json";
  ConnectionInfo  ci("", "POST", "1.1");
  std::string     expected = "\"updateContextAvailabilitySubscriptionResponse\" : {\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"bad value for 'isPattern'\"\n  },\n  \"subscriptionId\" : \"012345678901234567890123\"\n}\n";

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, UpdateContextAvailabilitySubscription, "updateContextAvailabilitySubscriptionRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* response - 
*/
TEST(UpdateContextAvailabilitySubscriptionRequest, response)
{
  UpdateContextAvailabilitySubscriptionResponse  ucas;
  ErrorCode                                      ec(SccBadRequest, "Reason", "Detail");
  UpdateContextAvailabilitySubscriptionResponse  ucas2(ec);
  std::string                                    render;
  std::string                                    check;
  std::string                                    expected1 = "<updateContextAvailabilitySubscriptionResponse>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n</updateContextAvailabilitySubscriptionResponse>\n";
  std::string                                    expected2 = "<updateContextAvailabilitySubscriptionResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Reason</reasonPhrase>\n    <details>Detail</details>\n  </errorCode>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n</updateContextAvailabilitySubscriptionResponse>\n";
  std::string                                    expected3 = "<updateContextAvailabilitySubscriptionResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>syntax error in duration string</reasonPhrase>\n  </errorCode>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n</updateContextAvailabilitySubscriptionResponse>\n";
  std::string                                    expected4 = "<updateContextAvailabilitySubscriptionResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>predetected error</reasonPhrase>\n  </errorCode>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n</updateContextAvailabilitySubscriptionResponse>\n";
  
  EXPECT_EQ(ucas2.errorCode.code, SccBadRequest);

  ucas.subscriptionId.set("012345678901234567890123");

  check = ucas.check(UpdateContextAvailabilitySubscription, XML, "", "", 0);
  EXPECT_EQ("OK", check);
  
  render = ucas.render(UpdateContextAvailabilitySubscription, XML, "", 0);
  EXPECT_EQ(expected1, render);

  ucas.errorCode.fill(SccBadRequest, "Reason", "Detail");
  render = ucas.render(UpdateContextAvailabilitySubscription, XML, "", 0);
  EXPECT_EQ(expected2, render);
  
  ucas.errorCode.fill(NO_ERROR_CODE, "", "");
  ucas.duration.set("ddd");
  check = ucas.check(UpdateContextAvailabilitySubscription, XML, "", "", 0);
  EXPECT_EQ(expected3, check);

  check = ucas.check(UpdateContextAvailabilitySubscription, XML, "", "predetected error", 0);
  EXPECT_EQ(expected4, check);
}
