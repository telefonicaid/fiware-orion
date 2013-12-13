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

#include "jsonParse/jsonRequest.h"
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"

#include "ngsi/ParseData.h"
#include "ngsi/StatusCode.h"
#include "ngsi/ErrorCode.h"
#include "ngsi10/NotifyContextRequest.h"
#include "ngsi10/NotifyContextResponse.h"

#include "testDataFromFile.h"



/* ****************************************************************************
*
* xml_ok - 
*/
TEST(NotifyContextRequest, xml_ok)
{
   ParseData       reqData;
   ConnectionInfo  ci("", "POST", "1.1");
   const char*     fileName = "ngsi10.notifyContextRequest.ok.valid.xml";

   EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

   lmTraceLevelSet(LmtDump, true);
   std::string result = xmlTreat(testBuf, &ci, &reqData, NotifyContext, "notifyContextRequest", NULL);
   EXPECT_EQ("OK", result);
   lmTraceLevelSet(LmtDump, false);

  NotifyContextRequest*  ncrP = &reqData.ncr.res;

  ncrP->present("");

  std::string rendered;
  std::string expected = "<notifyContextRequest>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n  <originator>http://localhost/test</originator>\n  <contextResponseList>\n    <contextElementResponse>\n      <contextElement>\n        <entityId type=\"Room\" isPattern=\"false\">\n          <id>ConferenceRoom</id>\n        </entityId>\n        <contextAttributeList>\n          <contextAttribute>\n            <name>temperature</name>\n            <type>Room</type>\n            <contextValue>10</contextValue>\n          </contextAttribute>\n          <contextAttribute>\n            <name>temperature</name>\n            <type>Room</type>\n            <contextValue>10</contextValue>\n          </contextAttribute>\n        </contextAttributeList>\n      </contextElement>\n      <statusCode>\n        <code>200</code>\n        <reasonPhrase>Ok</reasonPhrase>\n        <details>a</details>\n      </statusCode>\n    </contextElementResponse>\n  </contextResponseList>\n</notifyContextRequest>\n";

  rendered = ncrP->render(NotifyContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

  ncrP->present("");
  ncrP->release();
}



/* ****************************************************************************
*
* json_ok - 
*/
TEST(NotifyContextRequest, json_ok)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName     = "notifyContextRequest_ok.json";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  
  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, NotifyContext, "notifyContextRequest", NULL);
  EXPECT_EQ("OK", result);
  lmTraceLevelSet(LmtDump, false);

  //
  // With the data obtained, render, present and release methods are exercised
  //
  NotifyContextRequest*  ncrP = &reqData.ncr.res;

  ncrP->present("");

  std::string rendered;
  const char* expectedFile = "ngsi10.notifyContextRequest_ok.expected1.valid.json";

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), expectedFile)) << "Error getting test data from '" << expectedFile << "'";
  rendered = ncrP->render(NotifyContext, JSON, "");
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  ncrP->release();
}



/* ****************************************************************************
*
* xml_badIsPattern - 
*/
TEST(NotifyContextRequest, xml_badIsPattern)
{
   ParseData       reqData;
   ConnectionInfo  ci("", "POST", "1.1");
   const char*     fileName = "ngsi10.notifyContextRequest.isPattern.invalid.xml";
   std::string     expected = "<notifyContextResponse>\n  <responseCode>\n    <code>400</code>\n    <reasonPhrase>bad value for 'isPattern'</reasonPhrase>\n  </responseCode>\n</notifyContextResponse>\n";

   EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

   std::string result = xmlTreat(testBuf, &ci, &reqData, NotifyContext, "notifyContextRequest", NULL);
   EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* json_badIsPattern - 
*/
TEST(NotifyContextRequest, json_badIsPattern)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "notifyContextRequest_badIsPattern.json";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  
  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  std::string result   = jsonTreat(testBuf, &ci, &reqData, NotifyContext, "notifyContextRequest", NULL);
  std::string expected = "{\n  \"responseCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"bad value for 'isPattern'\"\n  }\n}\n";

  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* xml_invalidEntityIdAttribute - 
*/
TEST(NotifyContextRequest, xml_invalidEntityIdAttribute)
{
   ParseData       reqData;
   ConnectionInfo  ci("", "POST", "1.1");
   const char*     fileName = "ngsi10.notifyContextRequest.entityIdAttribute.invalid.xml";
   std::string     expected = "<notifyContextResponse>\n  <responseCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </responseCode>\n</notifyContextResponse>\n";

   EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

   std::string result = xmlTreat(testBuf, &ci, &reqData, NotifyContext, "notifyContextRequest", NULL);
   EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* predetectedError - 
*/
TEST(NotifyContextRequest, predetectedError)
{
   NotifyContextRequest ncr;
   std::string          expected = "<notifyContextResponse>\n  <responseCode>\n    <code>400</code>\n    <reasonPhrase>predetected error</reasonPhrase>\n  </responseCode>\n</notifyContextResponse>\n";
   std::string          out      = ncr.check(NotifyContext, XML, "", "predetected error", 0);

   EXPECT_EQ(expected, out);
}



/* ****************************************************************************
*
* Constructor - 
*/
TEST(NotifyContextResponse, Constructor)
{
   StatusCode sc(SccOk, "1", "2");
   NotifyContextResponse ncr(sc);
   EXPECT_EQ(SccOk, ncr.responseCode.code);
   ncr.present("");
   ncr.release();

   ErrorCode ec(SccOk, "3", "4");
   NotifyContextResponse ncr2(ec);
   EXPECT_EQ(SccOk, ncr2.responseCode.code);
}
