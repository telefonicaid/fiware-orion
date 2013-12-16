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
#include "rest/ConnectionInfo.h"
#include "jsonParse/jsonRequest.h"
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"

#include "testDataFromFile.h"



/* ****************************************************************************
*
* Tests
* - badLength_xml
* - badLength_json
* - invalidDuration_json
*
*/



/* ****************************************************************************
*
* badLength_xml - 
*/
TEST(UpdateContextSubscriptionRequest, badLength_xml)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.updateContextSubscription.subscriptionIdLength.invalid.xml";
  const char*     expected1 = "<updateContextSubscriptionResponse>\n  <subscribeError>\n    <subscriptionId>12345</subscriptionId>\n    <errorCode>\n      <code>400</code>\n      <reasonPhrase>bad length (24 chars expected)</reasonPhrase>\n    </errorCode>\n  </subscribeError>\n</updateContextSubscriptionResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &parseData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_STREQ(expected1, result.c_str());

  //
  // With the data obtained, render, present and release methods are exercised
  //
  UpdateContextSubscriptionRequest*  ucsrP = &parseData.ucsr.res;
  
  ucsrP->present(""); // No output

  std::string rendered;
  std::string checked;
  std::string expected2 = "<updateContextSubscriptionRequest>\n  <duration>P50Y</duration>\n  <restriction>\n    <attributeExpression>AttriTest</attributeExpression>\n    <scope>\n      <operationScope>\n        <type>st1</type>\n        <value>sv1</value>\n      </operationScope>\n      <operationScope>\n        <type>st2</type>\n        <value>sv2</value>\n      </operationScope>\n    </scope>\n  </restriction>\n  <subscriptionId>12345</subscriptionId>\n  <notifyConditions>\n    <notifyCondition>\n      <type>ONCHANGE</type>\n      <condValueList>\n        <condValue>CondValue3</condValue>\n        <condValue>CondValue4</condValue>\n      </condValueList>\n    </notifyCondition>\n  </notifyConditions>\n  <throttling>P5Y</throttling>\n</updateContextSubscriptionRequest>\n";
  std::string expected3 = "<updateContextSubscriptionResponse>\n  <subscribeError>\n    <subscriptionId>12345</subscriptionId>\n    <errorCode>\n      <code>400</code>\n      <reasonPhrase>FORCED ERROR</reasonPhrase>\n    </errorCode>\n  </subscribeError>\n</updateContextSubscriptionResponse>\n";
  std::string expected4 = "<updateContextSubscriptionResponse>\n  <subscribeError>\n    <subscriptionId>12345</subscriptionId>\n    <errorCode>\n      <code>400</code>\n      <reasonPhrase>syntax error in duration string</reasonPhrase>\n    </errorCode>\n  </subscribeError>\n</updateContextSubscriptionResponse>\n";

  rendered = ucsrP->render(UpdateContextSubscription, XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  checked  = ucsrP->check(UpdateContextSubscription, XML, "", "FORCED ERROR", 0);
  EXPECT_STREQ(expected3.c_str(), checked.c_str());

  ucsrP->duration.set("XXXYYYZZZ");
  checked  = ucsrP->check(UpdateContextSubscription, XML, "", "", 0);
  EXPECT_STREQ(expected4.c_str(), checked.c_str());

  ucsrP->present("");
  ucsrP->release();
}



/* ****************************************************************************
*
* badLength_json - 
*/
TEST(UpdateContextSubscriptionRequest, badLength_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName  = "updateContextSubscription_badLength.json";
  std::string     rendered;
  std::string     checked;
  const char*     expectedFile1 = "ngsi10.updateContextSubscriptionRequest_badLength.expected1.valid.json";
  const char*     expectedFile2 = "ngsi10.updateContextSubscriptionRequest_badLength.expected2.valid.json";
  const char*     expectedFile3 = "ngsi10.updateContextSubscriptionRequest_badLength.expected3.valid.json";
  const char*     expectedFile4 = "ngsi10.updateContextSubscriptionRequest_badLength.expected4.valid.json";
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), expectedFile1)) << "Error getting test data from '" << expectedFile1 << "'";
  lmTraceLevelSet(LmtDump, true);
  rendered = jsonTreat(testBuf, &ci, &parseData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  //
  // With the data obtained, render, present and release methods are exercised
  //
  UpdateContextSubscriptionRequest*  ucsrP = &parseData.ucsr.res;
  
  ucsrP->present(""); // No output

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), expectedFile2)) << "Error getting test data from '" << expectedFile2 << "'";
  rendered = ucsrP->render(UpdateContextSubscription, JSON, "");
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), expectedFile3)) << "Error getting test data from '" << expectedFile3 << "'";
  checked  = ucsrP->check(UpdateContextSubscription, JSON, "", "FORCED ERROR", 0);
  EXPECT_STREQ(expectedBuf, checked.c_str());

  ucsrP->duration.set("XXXYYYZZZ");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), expectedFile4)) << "Error getting test data from '" << expectedFile4 << "'";
  checked  = ucsrP->check(UpdateContextSubscription, JSON, "", "", 0);
  EXPECT_STREQ(expectedBuf, checked.c_str());

  ucsrP->present("");
  ucsrP->release();
}



/* ****************************************************************************
*
* invalidDuration_json - 
*/
TEST(UpdateContextSubscriptionRequest, invalidDuration_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName  = "updateContextSubscription_invalidDuration.json";
  std::string     expected  = "{\n  \"subscribeError\" : {\n    \"subscriptionId\" : \"9212ce4b0c214479be429e2b\",\n    \"errorCode\" : {\n      \"code\" : \"400\",\n      \"reasonPhrase\" : \"syntax error in duration string\"\n    }\n  }\n}\n";
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  std::string result = jsonTreat(testBuf, &ci, &parseData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_EQ(expected, result);
}
