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
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "jsonParse/jsonRequest.h"
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"

#include "testDataFromFile.h"



/* ****************************************************************************
*
* ok_xml - 
*/
TEST(SubscribeContextRequest, ok_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "subscribeContextRequest_ok.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", result);

  //
  // With the data obtained, render, present and release methods are exercised
  //
  SubscribeContextRequest*  scrP = &reqData.scr.res;
  
  scrP->present(""); // No output

  std::string rendered;
  std::string expected = "<subscribeContextRequest>\n  <entityIdList>\n    <entityId type=\"Room\" isPattern=\"false\">\n      <id>ConferenceRoom</id>\n    </entityId>\n    <entityId type=\"Room\" isPattern=\"false\">\n      <id>OfficeRoom</id>\n    </entityId>\n  </entityIdList>\n  <attributeList>\n    <attribute>temperature</attribute>\n    <attribute>lightstatus</attribute>\n  </attributeList>\n  <reference>http://127.0.0.1:1028</reference>\n  <duration>P5Y</duration>\n  <restriction>\n    <attributeExpression>testRestriction</attributeExpression>\n    <scope>\n      <operationScope>\n        <type>scope1</type>\n        <value>sval1</value>\n      </operationScope>\n      <operationScope>\n        <type>scope2</type>\n        <value>sval2</value>\n      </operationScope>\n    </scope>\n  </restriction>\n  <contextAttributeList>\n    <notifyCondition>\n      <type>ONCHANGE</type>\n      <attributeList>\n        <attribute>temperature</attribute>\n        <attribute>lightstatus</attribute>\n      </attributeList>\n      <restriction>restriction</restriction>\n    </notifyCondition>\n  </contextAttributeList>\n  <throttling>P5Y</throttling>\n</subscribeContextRequest>\n";

  rendered = scrP->render(QueryContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

  scrP->release();
}



/* ****************************************************************************
*
* ok_json - 
*/
TEST(SubscribeContextRequest, ok_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "subscribeContextRequest_ok.json";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_EQ("OK", result);
  lmTraceLevelSet(LmtDump, false);


  //
  // With the data obtained, render, present and release methods are exercised
  //
  SubscribeContextRequest*  scrP = &parseData.scr.res;
  
  scrP->present(""); // No output

  std::string rendered;
  std::string expected = "\"subscribeContextRequest\" : {\n  \"entities\" : [\n    {\n      \"type\" : \"Room\",\n      \"isPattern\" : \"false\",\n      \"id\" : \"ConferenceRoom\"\n    },\n    {\n      \"type\" : \"Room\",\n      \"isPattern\" : \"false\",\n      \"id\" : \"OfficeRoom\"\n    }\n  ]\n  \"attributeList\" : {\n    \"attribute\" : \"temperature\",\n    \"attribute\" : \"occupancy\",\n    \"attribute\" : \"lightstatus\"\n  }\n  \"reference\" : \"127.0.0.1\"\n  \"duration\" : \"P5Y\",\n  \"restriction\" : {\n    \"attributeExpression\" : \"testRestriction\"\n    \"scope\" : {\n      \"operationScope\" : {\n        \"type\" : \"t1\"\n        \"value\" : \"v1\"\n      }\n      \"operationScope\" : {\n        \"type\" : \"t2\"\n        \"value\" : \"v2\"\n      }\n    }\n  }\n  \"contextAttributeList\" : {\n    \"notifyCondition\" : {\n      \"type\" : \"ONTIMEINTERVAL\"\n      \"attributeList\" : {\n        \"attribute\" : \"temperature\",\n        \"attribute\" : \"lightstatus\"\n      }\n      \"restriction\" : \"restriction\"\n    }\n  }\n  \"throttling\" : \"P5Y\"\n}\n";

  rendered = scrP->render(SubscribeContext, JSON, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

  scrP->release();
}



/* ****************************************************************************
*
* invalidDuration_xml - 
*/
TEST(SubscribeContextRequest, invalidDuration_xml)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "subscribeContextRequest_invalidDuration.xml";
  const char*     expected = "<subscribeContextResponse>\n  <subscribeError>\n    <errorCode>\n      <code>400</code>\n      <reasonPhrase>invalid payload</reasonPhrase>\n      <details>syntax error in duration string</details>\n    </errorCode>\n  </subscribeError>\n</subscribeContextResponse>\n";
  XmlRequest*     reqP;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", &reqP);
  lmTraceLevelSet(LmtDump, false);

  reqP->release(&parseData);
  EXPECT_STREQ(expected, result.c_str());
}



/* ****************************************************************************
*
* badIsPattern_json - 
*/
TEST(SubscribeContextRequest, badIsPattern_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "subscribeContextRequest_badIsPattern.json";
  const char*     expected = "{\n  \"subscribeError\" : {\n    \"errorCode\" : {\n      \"code\" : \"400\",\n      \"reasonPhrase\" : \"invalid payload\",\n      \"details\" : \"bad value for 'isPattern'\"\n    }\n  }\n}\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  std::string result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* invalidDuration_json - 
*/
TEST(SubscribeContextRequest, invalidDuration_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "subscribeContextRequest_invalidDuration.json";
  const char*     expected = "{\n  \"subscribeError\" : {\n    \"errorCode\" : {\n      \"code\" : \"400\",\n      \"reasonPhrase\" : \"invalid payload\",\n      \"details\" : \"syntax error in duration string\"\n    }\n  }\n}\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  std::string result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* invalidEntityIdAttribute_xml - 
*
* FIXME P5: invalid attributes in EntityId are found but not reported
*/
TEST(SubscribeContextRequest, invalidEntityIdAttribute_xml)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "subscribeContextRequest_invalidEntityIdAttribute.xml";
  const char*     expected = "OK";
  XmlRequest*     reqP;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", &reqP);

  reqP->release(&parseData);
  EXPECT_STREQ(expected, result.c_str());
}
