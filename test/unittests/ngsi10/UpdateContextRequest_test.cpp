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
#include "jsonParse/jsonRequest.h"

#include "unittest.h"



/* ****************************************************************************
*
* Tests
* - ok_json
* - badIsPattern_json
*/



/* ****************************************************************************
*
* ok_json -
*/
TEST(UpdateContextRequest, ok_json)
{
   ParseData       reqData;
   ConnectionInfo  ci("", "POST", "1.1");
   const char*     infile = "ngsi10.updateContext.ok.valid.json";

   utInit();

   EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

   ci.inMimeType  = JSON;
   ci.outMimeType = JSON;

   lmTraceLevelSet(LmtDump, true);
   std::string result = jsonTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);
   lmTraceLevelSet(LmtDump, false);

   EXPECT_EQ("OK", result);

   //
   // With the data obtained, render, present and release methods are exercised
   //
   UpdateContextRequest*  upcrP = &reqData.upcr.res;

   upcrP->release();
   utExit();
}



/* ****************************************************************************
*
* badIsPattern_json -
*/
TEST(UpdateContextRequest, badIsPattern_json)
{
   ParseData       parseData;
   ConnectionInfo  ci("", "POST", "1.1");
   const char*     infile  = "ngsi10.updateContextRequest.badIsPattern.invalid.json";
   const char*     outfile = "ngsi10.updateContextResponse.badIsPattern.invalid.json";
   JsonRequest*    reqP;

   utInit();

   ci.inMimeType  = JSON;
   ci.outMimeType = JSON;

   EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
   EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile;

   std::string out = jsonTreat(testBuf, &ci, &parseData, UpdateContext, "updateContextRequest", &reqP);
   EXPECT_STREQ(expectedBuf, out.c_str());
   reqP->release(&parseData);

   utExit();
}
