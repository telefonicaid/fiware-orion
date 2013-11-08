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

#include "ngsi/SubscriptionId.h"
#include "ngsi9/UnsubscribeContextAvailabilityRequest.h"



/* ****************************************************************************
*
* constructorsAndCheck -
*/
TEST(UnsubscribeContextAvailabilityRequest, constructorAndCheck)
{
  UnsubscribeContextAvailabilityRequest ucar1;
  SubscriptionId                        subId("012345678901234567890123");
  UnsubscribeContextAvailabilityRequest ucar2(subId);

  EXPECT_EQ("", ucar1.subscriptionId.get());
  EXPECT_EQ("012345678901234567890123", ucar2.subscriptionId.get());

  std::string   out;
  std::string   expected1 = "<unsubscribeContextAvailabilityResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>Forced Error</reasonPhrase>\n  </statusCode>\n</unsubscribeContextAvailabilityResponse>\n";
  std::string   expected2 = "<unsubscribeContextAvailabilityResponse>\n  <subscriptionId>1</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>bad length (24 chars expected)</reasonPhrase>\n  </statusCode>\n</unsubscribeContextAvailabilityResponse>\n";
  std::string   expected3 = "OK";

  out = ucar1.check(UnsubscribeContextAvailability, XML, "", "Forced Error", 0);
  EXPECT_EQ(expected1, out);

  ucar1.subscriptionId.set("1");
  out = ucar1.check(UnsubscribeContextAvailability, XML, "", "", 0);
  EXPECT_EQ(expected2, out);

  out = ucar2.check(UnsubscribeContextAvailability, XML, "", "", 0);
  EXPECT_EQ(expected3, out);
}



/* ****************************************************************************
*
* badSubscriptionId_xml - 
*/
TEST(UnsubscribeContextAvailabilityRequest, badSubscriptionId_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "unsubscribeContextAvailabilityRequest_badSubscriptionId.xml";
  std::string     rendered;
  std::string     expected = "<unsubscribeContextAvailabilityResponse>\n  <subscriptionId>12345</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>bad length (24 chars expected)</reasonPhrase>\n  </statusCode>\n</unsubscribeContextAvailabilityResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, UnsubscribeContextAvailability, "unsubscribeContextAvailabilityRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_STREQ(expected.c_str(), result.c_str());

  UnsubscribeContextAvailabilityRequest*  ucarP = &reqData.ucar.res;
  
  ucarP->release();
}



/* ****************************************************************************
*
* badSubscriptionId_json - 
*/
TEST(UnsubscribeContextAvailabilityRequest, badSubscriptionId_json)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "unsubscribeContextAvailabilityRequest_badSubscriptionId.json";
  std::string     expected = "{\n  \"subscriptionId\" : \"12345\",\n  \"statusCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"bad length (24 chars expected)\"\n  }\n}\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  
  ci.inFormat  = JSON;
  ci.outFormat = JSON;
  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, UnsubscribeContextAvailability, "unsubscribeContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result);

  lmTraceLevelSet(LmtDump, false);

  UnsubscribeContextAvailabilityRequest*  ucarP = &reqData.ucar.res;

  ucarP->release();
}
