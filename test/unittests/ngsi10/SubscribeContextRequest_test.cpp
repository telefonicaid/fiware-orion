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
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"

#include "testDataFromFile.h"



/* ****************************************************************************
*
* Tests
* - ok
*
*/



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
* invalidDuration_xml - 
*/
TEST(SubscribeContextRequest, invalidDuration_xml)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "subscribeContextRequest_invalidDuration.xml";
  XmlRequest*     reqP;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", &reqP);
  lmTraceLevelSet(LmtDump, false);

  reqP->release(&parseData);
  EXPECT_STREQ("syntax error in duration string", result.c_str());
}
