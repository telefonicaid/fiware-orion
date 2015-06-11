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
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "rest/ConnectionInfo.h"
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
  ParseData       parseData;
  const char*     infile1  = "ngsi9.registerContextRequest.parseError.invalid.xml"; 
  const char*     infile2  = "ngsi9.registerContextRequest.ok.valid.xml"; 
  const char*     infile3  = "ngsi9.registerContextRequest.errorInFirstLine.invalid.xml";
  const char*     outfile1 = "orion.error.parseError.valid.xml";
  const char*     outfile2 = "orion.error.noRequestTreatingObjectFound.valid.xml";
  const char*     outfile3 = "orion.error.parseError.valid.xml";
  const char*     outfile4 = "ngsi9.registerContextResponse.invalidPayload.valid.xml";
  std::string     out;

  utInit();

  // Parse Error
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile1)) << "Error getting test data from '" << infile1 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  // Request not found
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile2)) << "Error getting test data from '" << infile2 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  ci2.inFormat  = XML;
  ci2.outFormat = XML;
  out  = xmlTreat(testBuf, &ci2, &parseData, (RequestType) (RegisterContext + 1000), "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  // Error in first line '<?xml version="1.0" encoding="UTF-8"?>'
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile3)) << "Error getting test data from '" << infile3 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  ci3.inFormat  = XML;
  ci3.outFormat = XML;
  out  = xmlTreat(testBuf, &ci3, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  // Payload word differs
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile2)) << "Error getting test data from '" << infile2 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile4)) << "Error getting test data from '" << outfile4 << "'";
  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "discovery", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* emptyPayload - 
*/
TEST(xmlRequest, emptyPayload)
{
  ConnectionInfo  ci("/ngsi/registerContext", "POST", "1.1");
  ParseData       parseData;
  const char*     payload = "";
  std::string     out;

  utInit();

  out  = xmlTreat(payload, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}
