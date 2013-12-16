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

#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "xmlParse/xmlRequest.h"

#include "unittest.h"



/* ****************************************************************************
*
* parseError - 
*/
TEST(xmlRequest, parseError)
{
  ConnectionInfo  ci("/ngsi/registerContext", "POST", "1.1");
  ConnectionInfo  ci2("/ngsi/registerContext123", "POST", "1.1");
  ConnectionInfo  ci3("/ngsi/registerContext", "POST", "1.1");
  ConnectionInfo  ci4("/version", "POST", "1.1");
  const char*     fileName  = "ngsi9.registerContextRequest.parseError.invalid.xml"; 
  const char*     fileName2 = "ngsi9.registerContextRequest.ok.valid.xml"; 
  const char*     fileName3 = "ngsi9.registerContextRequest.errorInFirstLine.invalid.xml";

  ParseData       parseData;
  std::string     expected  = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Parse Error</reasonPhrase>\n</orionError>\n";
  std::string     expected2 = "<orionError>\n  <code>400</code>\n  <reasonPhrase>no request treating object found</reasonPhrase>\n  <details>Sorry, no request treating object found for RequestType '', method 'POST'</details>\n</orionError>\n";
  std::string     expected3 = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Parse Error</reasonPhrase>\n</orionError>\n";
  std::string     expected4 = "<registerContextResponse>\n  <registrationId>0</registrationId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Invalid payload</reasonPhrase>\n    <details>Expected 'discovery' payload, got 'registerContextRequest'</details>\n  </errorCode>\n</registerContextResponse>\n";
  std::string     expected5 = "not ok";
  std::string     out;

  utInit();

  // Parse Error
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expected, out);

  // Request not found
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName2)) << "Error getting test data from '" << fileName2 << "'";
  ci2.inFormat  = XML;
  ci2.outFormat = XML;
  out  = xmlTreat(testBuf, &ci2, &parseData, (RequestType) (RegisterContext + 1000), "registerContextRequest", NULL);
  EXPECT_EQ(expected2, out);

  // Error in first line '<?xml version="1.0" encoding="UTF-8"?>'
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName3)) << "Error getting test data from '" << fileName3 << "'";
  ci3.inFormat  = XML;
  ci3.outFormat = XML;
  out  = xmlTreat(testBuf, &ci3, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expected3, out);

  // Payload word differs
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName2)) << "Error getting test data from '" << fileName2 << "'";
  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "discovery", NULL);
  EXPECT_EQ(expected4, out);

  utExit();
}
