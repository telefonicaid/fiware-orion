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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "jsonParse/jsonRequest.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/xmlRequest.h"

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
* xml_ok - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_ok)
{
  ParseData       reqData;
  const char*     infile  = "ngsi9.subscribeContextAvailabilityRequest.subscriptionIdNot24Chars.invalid.xml";
  const char*     outfile = "ngsi9.subscribeContextAvailabilityRequest.subscriptionIdNot24Chars2.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");
  
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_EQ("OK", result) << "this test should be OK";

  SubscribeContextAvailabilityRequest*  scarP = &reqData.scar.res;
  std::string                           out;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  out = scarP->render(SubscribeContextAvailability, XML, "");
  EXPECT_STREQ(expectedBuf, out.c_str());
}



/* ****************************************************************************
*
* json_ok - 
*/
TEST(SubscribeContextAvailabilityRequest, json_ok)
{
  ParseData       reqData;
  const char*     infile = "subscribeContextAvailabilityRequest_ok.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

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
  const char*     infile = "ngsi9.subscribeContextAvailabilityRequest.isPattern.invalid.xml";
  std::string     expected = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>000000000000000000000000</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad value for 'isPattern'</reasonPhrase>\n  </errorCode>\n</subscribeContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

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
  const char*     infile = "subscribeContextAvailabilityRequest_badIsPattern.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

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
  const char*     infile = "ngsi9.subscribeContextAvailabilityRequest.entityId.invalid.xml";
  std::string     expected = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>000000000000000000000000</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</subscribeContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

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
  const char*     infile = "ngsi9.subscribeContextAvailabilityRequest.noEntity.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");
  std::string     expected = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>000000000000000000000000</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>No entities</reasonPhrase>\n  </errorCode>\n</subscribeContextAvailabilityResponse>\n";

  ci.inFormat      = XML;
  ci.outFormat     = XML;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* json_noEntityId - 
*/
TEST(SubscribeContextAvailabilityRequest, json_noEntityId)
{
  ParseData       reqData;
  const char*     infile = "subscribeContextAvailabilityRequest_noEntityId.json";
  ConnectionInfo  ci("", "POST", "1.1");
  std::string     expected = "{\n  \"subscriptionId\" : \"000000000000000000000000\",\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"No entities\"\n  }\n}\n";

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* xml_entityIdTypeAsBothFieldAndAttribute - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_entityIdTypeAsBothFieldAndAttribute)
{
  ParseData       reqData;
  const char*     infile = "ngsi9.subscribeContextAvailabilityRequest.entityIdTypeAsBothFieldAndAttribute.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = XML;
  ci.outFormat     = XML;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

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
  const char*     infile = "ngsi9.subscribeContextAvailabilityRequest.entityIdIsPatternAsBothFieldAndAttribute.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat      = XML;
  ci.outFormat     = XML;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should NOT be OK";
}



/* ****************************************************************************
*
* xml_noReference - 
*/
TEST(SubscribeContextAvailabilityRequest, xml_noReference)
{
  ParseData       reqData;
  const char*     infile = "ngsi9.subscribeContextAvailabilityRequest.noReference.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");
  std::string     expected = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>000000000000000000000000</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty Reference</reasonPhrase>\n  </errorCode>\n</subscribeContextAvailabilityResponse>\n";

  ci.inFormat      = XML;
  ci.outFormat     = XML;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* json_badDuration - 
*/
TEST(SubscribeContextAvailabilityRequest, json_badDuration)
{
  ParseData       reqData;
  const char*     infile = "subscribeContextAvailabilityRequest_badDuration.json";
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     expected = "{\n  \"subscriptionId\" : \"000000000000000000000000\",\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"syntax error in duration string\"\n  }\n}\n";

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, SubscribeContextAvailability, "subscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result) << "this test should NOT be OK";
}
