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

#include "jsonParse/jsonRequest.h"

#include "ngsi/ParseData.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/NotifyContextRequest.h"
#include "ngsi10/NotifyContextResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* json_ok -
*/
TEST(NotifyContextRequest, json_ok)
{
  ParseData              reqData;
  ConnectionInfo         ci("", "POST", "1.1");
  NotifyContextRequest*  ncrP      = &reqData.ncr.res;
  const char*            infile    = "notifyContextRequest_ok.json";
  const char*            outfile   = "ngsi10.notifyContextRequest_ok.expected1.valid.json";
  std::string            rendered;

  utInit();

  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, NotifyContext, NULL);
  EXPECT_EQ("OK", result);
  lmTraceLevelSet(LmtDump, false);

  //
  // With the data obtained, render, present and release methods are exercised
  //
  std::vector<std::string> emptyV;
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  rendered = ncrP->toJsonV1(false, emptyV, false, emptyV);
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  ncrP->release();

  utExit();
}



/* ****************************************************************************
*
* json_badIsPattern -
*/
TEST(NotifyContextRequest, json_badIsPattern)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile   = "ngsi10.notifyContextRequest.badIsPattern.invalid.json";
  const char*     outfile  = "ngsi10.notifyContextResponse.badIsPattern.valid.json";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  std::string out = jsonTreat(testBuf, &ci, &reqData, NotifyContext, NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* Constructor -
*/
TEST(NotifyContextResponse, Constructor)
{
  StatusCode sc(SccOk, "2");
  NotifyContextResponse ncr(sc);

  utInit();

  EXPECT_EQ(SccOk, ncr.responseCode.code);
  ncr.release();

  StatusCode ec(SccOk, "4");
  NotifyContextResponse ncr2(ec);
  EXPECT_EQ(SccOk, ncr2.responseCode.code);

  utExit();
}



/* ****************************************************************************
*
* json_render -
*/
TEST(NotifyContextRequest, json_render)
{
  const char*              filename1  = "ngsi10.notifyContextRequest.jsonRender1.valid.json";
  const char*              filename2  = "ngsi10.notifyContextRequest.jsonRender2.valid.json";
  const char*              filename3  = "ngsi10.notifyContextRequest.jsonRender3.valid.json";
  NotifyContextRequest*    ncrP;
  ContextElementResponse*  cerP;
  std::string              rendered;

  utInit();

  // Preparation
  ncrP = new NotifyContextRequest();
  ncrP->subscriptionId.set("012345678901234567890123");
  ncrP->originator.set("http://www.tid.es/NotifyContextRequestUnitTest");

  std::vector<std::string> emptyV;

  // 1. Without ContextResponseList
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  rendered = ncrP->toJsonV1(false, emptyV, false, emptyV);
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 2. With ContextResponseList
  cerP = new ContextElementResponse();
  cerP->entity.fill("E01", "EType", "false");
  ncrP->contextElementResponseVector.push_back(cerP);
  cerP->statusCode.fill(SccOk);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  rendered = ncrP->toJsonV1(false, emptyV, false, emptyV);
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 3. ContextResponseList with two instances
  cerP = new ContextElementResponse();
  cerP->entity.fill("E02", "EType", "false");
  ncrP->contextElementResponseVector.push_back(cerP);
  cerP->statusCode.fill(SccOk);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename3)) << "Error getting test data from '" << filename3 << "'";
  rendered = ncrP->toJsonV1(false, emptyV, false, emptyV);
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  utExit();
}
