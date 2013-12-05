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

#include "testDataFromFile.h"
#include "common/globals.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/xmlRequest.h"
#include "jsonParse/jsonRequest.h"
#include "xmlParse/xmlParse.h"



/* ****************************************************************************
*
* Tests
* - ok_xml
* - ok_json
* - badIsPattern_json
*/



/* ****************************************************************************
*
* ok_xml - 
*/
TEST(UpdateContextRequest, ok_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.updateContext.valid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", result);

  //
  // With the data obtained, render, present and release methods are exercised
  //
  UpdateContextRequest*  upcrP = &reqData.upcr.res;
  
  upcrP->present(""); // No output

  std::string rendered;
  std::string checked;
  std::string expected = "<updateContextRequest>\n  <contextElementList>\n    <contextElement>\n      <entityId type=\"Room\" isPattern=\"false\">\n        <id>ConferenceRoom</id>\n      </entityId>\n      <contextAttributeList>\n        <contextAttribute>\n          <name>temperature</name>\n          <type>degree</type>\n          <contextValue>c23</contextValue>\n          <metadata>\n            <contextMetadata>\n              <name>attributeValueIdentifier1</name>\n              <type>long</type>\n              <value>1</value>\n            </contextMetadata>\n            <contextMetadata>\n              <name>attributeValueIdentifier2</name>\n              <type>long</type>\n              <value>2</value>\n            </contextMetadata>\n            <contextMetadata>\n              <name>attributeValueIdentifier3</name>\n              <type>long</type>\n              <value>3</value>\n            </contextMetadata>\n          </metadata>\n        </contextAttribute>\n        <contextAttribute>\n          <name>lightstatus</name>\n          <type>light</type>\n          <contextValue>d23</contextValue>\n          <metadata>\n            <contextMetadata>\n              <name>attributeValueIdentifier1</name>\n              <type>long</type>\n              <value>1</value>\n            </contextMetadata>\n            <contextMetadata>\n              <name>attributeValueIdentifier2</name>\n              <type>long</type>\n              <value>2</value>\n            </contextMetadata>\n            <contextMetadata>\n              <name>attributeValueIdentifier3</name>\n              <type>long</type>\n              <value>3</value>\n            </contextMetadata>\n          </metadata>\n        </contextAttribute>\n      </contextAttributeList>\n    </contextElement>\n  </contextElementList>\n  <updateAction>APPEND</updateAction>\n</updateContextRequest>\n";
  std::string expected2 = "<updateContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad request</reasonPhrase>\n    <details>FORCED ERROR</details>\n  </errorCode>\n</updateContextResponse>\n";
  std::string expected3 = "<updateContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad request</reasonPhrase>\n    <details>invalid update action type: 'invalid'</details>\n  </errorCode>\n</updateContextResponse>\n";

  rendered = upcrP->render(UpdateContext, XML, "");
  EXPECT_EQ(expected, rendered);

  checked  = upcrP->check(UpdateContext, XML, "", "FORCED ERROR", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  upcrP->updateActionType.set("invalid");
  checked  = upcrP->check(RegisterContext, XML, "", "", 0); // Cannot use UpdateContext here ... [ UpdateActionType.cpp: if (requestType == UpdateContext) // FIXME: this is just to make harness test work ]
  EXPECT_STREQ(expected3.c_str(), checked.c_str());

  upcrP->release();
}



/* ****************************************************************************
*
* ok_json - 
*/
TEST(UpdateContextRequest, ok_json)
{
   ParseData       reqData;
   ConnectionInfo  ci("", "POST", "1.1");
   const char*     fileName = "updateContext_ok.json";

   EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

   ci.inFormat  = JSON;
   ci.outFormat = JSON;

   lmTraceLevelSet(LmtDump, true);
   std::string result = jsonTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);
   lmTraceLevelSet(LmtDump, false);

   EXPECT_EQ("OK", result);

   //
   // With the data obtained, render, present and release methods are exercised
   //
   UpdateContextRequest*  upcrP = &reqData.upcr.res;

   upcrP->present(""); // No output
   upcrP->release();
}



/* ****************************************************************************
*
* badIsPattern_json - 
*/
TEST(UpdateContextRequest, badIsPattern_json)
{
   ParseData       parseData;
   ConnectionInfo  ci("", "POST", "1.1");
   const char*     fileName = "updateContext_badIsPattern.json";
   std::string     expected = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"Bad request\",\n    \"details\" : \"bad value for 'isPattern'\"\n  }\n}\n";
   JsonRequest*    reqP;

   ci.inFormat  = JSON;
   ci.outFormat = JSON;

   EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

   std::string result = jsonTreat(testBuf, &ci, &parseData, UpdateContext, "updateContextRequest", &reqP);
   reqP->release(&parseData);
   EXPECT_EQ(expected, result);
}
