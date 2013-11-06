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

#include "testDataFromFile.h"



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
* xml_ok - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_ok)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_ok.xml";
  ConnectionInfo  ci("", "POST", "1.1");
  
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ("OK", result) << "this test should be OK";

  SubscribeContextAvailabilityRequest*  scarP = &reqData.scar.res;
  std::string                           out;
  std::string                           expected = "<subscribeContextAvailabilityRequest>\n  <entityIdList>\n    <entityId type=\"Room\" isPattern=\"false\">\n      <id>ConferenceRoom</id>\n    </entityId>\n    <entityId type=\"Room\" isPattern=\"false\">\n      <id>OfficeRoom</id>\n    </entityId>\n  </entityIdList>\n  <attributeList>\n    <attribute>temperature</attribute>\n    <attribute>occupancy</attribute>\n    <attribute>lightstatus</attribute>\n  </attributeList>\n  <reference>http://10.1.1.1:80/test/interfaceNotification\n\t</reference>\n  <duration>PT1M</duration>\n  <restriction>\n    <attributeExpression>ATTR_EXPR</attributeExpression>\n    <scope>\n      <operationScope>\n        <type>st1</type>\n        <value>st1</value>\n      </operationScope>\n    </scope>\n  </restriction>\n  <subscriptionId>12458896</subscriptionId>\n</subscribeContextAvailabilityRequest>\n";

  out = scarP->render(SubscribeContextAvailability, XML, "");
  EXPECT_EQ(expected, out);
}



/* ****************************************************************************
*
* json_ok - 
*/
TEST(SubscribeContextAvailabilityRequest, json_ok)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_ok.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ("OK", result) << "this test should be OK";
}



/* ****************************************************************************
*
* xml_badIsPattern - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_badIsPattern)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_badIsPattern.xml";
  std::string     expected = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad value for 'isPattern'</reasonPhrase>\n  </errorCode>\n</subscribeContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result) << "this test should be BADISPATTERN";
}



/* ****************************************************************************
*
* json_badIsPattern - 
*/
TEST(SubscribeContextAvailabilityRequest, json_badIsPattern)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_badIsPattern.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should NOT be OK";
}



/* ****************************************************************************
*
* xml_badEntityId - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_badEntityId)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_badEntityId.xml";
  std::string     expected = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</subscribeContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* xml_noEntityId - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_noEntityId)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_noEntityId.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = XML;
  ci.outFormat     = XML;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should NOT be OK";
}



/* ****************************************************************************
*
* json_noEntityId - 
*/
TEST(SubscribeContextAvailabilityRequest, json_noEntityId)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_noEntityId.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should NOT be OK";
}



/* ****************************************************************************
*
* xml_entityIdTypeAsBothFieldAndAttribute - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_entityIdTypeAsBothFieldAndAttribute)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_entityIdTypeAsBothFieldAndAttribute.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = XML;
  ci.outFormat     = XML;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should NOT be OK";
}



/* ****************************************************************************
*
* xml_entityIdIsPatternAsBothFieldAndAttribute - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_entityIdIsPatternAsBothFieldAndAttribute)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_entityIdIsPatternAsBothFieldAndAttribute.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = XML;
  ci.outFormat     = XML;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should NOT be OK";
}



/* ****************************************************************************
*
* json_badDuration - 
*/
TEST(SubscribeContextAvailabilityRequest, json_badDuration)
{
  ParseData       reqData;
  const char*     fileName = "subscribeContextAvailabilityRequest_badDuration.json";
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     expected = "{\n  \"subscriptionId\" : \"No Subscription ID\"\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"syntax error in duration string\"\n  }\n}\n";

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result) << "this test should NOT be OK";
}
