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

#include "common/globals.h"
#include "jsonParse/jsonRequest.h"

#include "unittest.h"



/* ****************************************************************************
*
* Tests
* - badLength_json
* - invalidDuration_json
*
*/



/* ****************************************************************************
*
* badLength_json -
*/
TEST(UpdateContextSubscriptionRequest, badLength_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  std::string     out;
  const char*     infile   = "ngsi10.updateContextSubscriptionRequest.badLength.invalid.json";
  const char*     outfile1 = "ngsi10.updateContextSubscriptionRequest.badLength.expected1.valid.json";
  const char*     outfile3 = "ngsi10.updateContextSubscriptionRequest.badLength.expected3.valid.json";
  const char*     outfile4 = "ngsi10.updateContextSubscriptionRequest.badLength.expected4.valid.json";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  lmTraceLevelSet(LmtDump, true);
  out = jsonTreat(testBuf, &ci, &parseData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_STREQ(expectedBuf, out.c_str());

  //
  // With the data obtained, render, present and release methods are exercised
  //
  UpdateContextSubscriptionRequest*  ucsrP = &parseData.ucsr.res;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  out  = ucsrP->check("FORCED ERROR", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucsrP->duration.set("XXXYYYZZZ");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile4)) << "Error getting test data from '" << outfile4 << "'";
  out  = ucsrP->check("", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucsrP->release();

  utExit();
}



/* ****************************************************************************
*
* invalidDuration_json -
*/
TEST(UpdateContextSubscriptionRequest, invalidDuration_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile   = "ngsi10.updateContextSubscriptionRequest.duration.invalid.json";
  const char*     outfile  = "ngsi10.updateContextSubscriptionResponse.invalidDuration.valid.json";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  std::string out = jsonTreat(testBuf, &ci, &parseData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleOkJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationCircleOkJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.circleOk.postponed.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationCircleInvertedJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.circleInverted.postponed.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedBadValueJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationCircleInvertedBadValueJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.circleInvertedBadValue.postponed.json";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.circleInvertedBadValue.valid.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleZeroRadiusJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationCircleZeroRadiusJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.circleZeroRadius.postponed.json";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.circleZeroRadius.valid.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOkJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonOkJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonOk.postponed.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonInvertedJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonInverted.postponed.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedBadValueJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonInvertedBadValueJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonInvertedBadValue.postponed.json";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.polygonInvertedBadValue.valid.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonNoVerticesJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonNoVerticesJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonNoVertices.postponed.json";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.polygonNoVertices.valid.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOneVertexJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonOneVertexJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonOneVertex.postponed.json";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.polygonOneVertex.valid.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonTwoVerticesJson -
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonTwoVerticesJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonTwoVertices.postponed.json";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.polygonTwoVertices.valid.json";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
