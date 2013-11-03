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
TEST(UnsubscribeContextAvailabilityRequest, badSubscriptionId_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "unsubscribeContextAvailabilityRequest_badSubscriptionId.xml";
  std::string     rendered;
  std::string     expected = "bad length (24 chars expected)";

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
  std::string     expected = "bad length (24 chars expected)";

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
