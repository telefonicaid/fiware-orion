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

#include "testDataFromFile.h"
#include "common/globals.h"
#include "ngsi/ParseData.h"
#include "jsonParse/jsonRequest.h"

#include "unittest.h"



/* ****************************************************************************
*
* badSubscriptionId_json -
*/
TEST(UnsubscribeContextRequest, badSubscriptionId_json)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile   = "ngsi10.unsubscribeContextRequest.badSubscriptionId.invalid.json";
  std::string     out;
  const char*     outfile2 = "ngsi10.unsubscribeContextResponse.badSubscriptionId2.valid.json";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;
  lmTraceLevelSet(LmtDump, true);
  out = jsonTreat(testBuf, &ci, &reqData, UnsubscribeContext, NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_STREQ("OK", out.c_str());

  UnsubscribeContextRequest*  ucrP = &reqData.uncr.res;


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  out = ucrP->toJsonV1();
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucrP->release();
}
