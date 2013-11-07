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
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"
#include "jsonParse/jsonRequest.h"


/* ****************************************************************************
*
* badSubscriptionId_xml - 
*/
TEST(UnsubscribeContextRequest, badSubscriptionId_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "unsubscribeContextRequest_badSubscriptionId.xml";
  std::string     rendered;
  std::string     expected = "<unsubscribeContextResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>Invalid Subscription Id</reasonPhrase>\n    <details>12345D</details>\n  </statusCode>\n</unsubscribeContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, UnsubscribeContext, "unsubscribeContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_STREQ(expected.c_str(), result.c_str());

  //
  // With the data obtained, render, present and release methods are exercised
  //
  UnsubscribeContextRequest*  ucrP = &reqData.uncr.res;
  
  ucrP->present(""); // No output

  expected = "<unsubscribeContextRequest>\n  <subscriptionId>12345D</subscriptionId>\n</unsubscribeContextRequest>\n";
  rendered = ucrP->render(UnsubscribeContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

  ucrP->release();
}



/* ****************************************************************************
*
* badSubscriptionId_json - 
*/
TEST(UnsubscribeContextRequest, badSubscriptionId_json)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "unsubscribeContextRequest_badSubscriptionId.json";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  
  ci.inFormat  = JSON;
  ci.outFormat = JSON;
  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, UnsubscribeContext, "unsubscribeContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  UnsubscribeContextRequest*  ucrP = &reqData.uncr.res;

  ucrP->present("");
  std::string expected = "{\n  \"subscriptionId\" : \"012345678901234567890123\"\n}\n";
  std::string rendered = ucrP->render(UnsubscribeContext, JSON, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

  ucrP->release();

}
