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
#include "ngsi/ParseData.h"
#include "jsonParse/jsonRequest.h"

#include "unittest.h"



/* ****************************************************************************
*
* ok_json -
*/
TEST(SubscribeContextRequest, ok_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.subscribeContextRequest.ok.valid.json";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_EQ("OK", result);
  lmTraceLevelSet(LmtDump, false);


  //
  // With the data obtained present and release methods are exercised
  //
  SubscribeContextRequest*  scrP    = &parseData.scr.res;
  scrP->release();

  utExit();
}



/* ****************************************************************************
*
* badIsPattern_json -
*/
TEST(SubscribeContextRequest, badIsPattern_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.subscribeContextRequest.badIsPattern.invalid.json";
  const char*     outfile = "ngsi10.subscribeContextResponse.badIsPattern.valid.json";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  std::string out = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* invalidDuration_json -
*/
TEST(SubscribeContextRequest, invalidDuration_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.subscribeContextRequest.duration.invalid.json";
  const char*     outfile = "ngsi10.subscribeContextResponse.durationInvalid.valid.json";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  std::string out = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleOkJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationCircleOkJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.circleOk.postponed.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationCircleInvertedJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.circleInverted.postponed.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedBadValueJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationCircleInvertedBadValueJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.circleInvertedBadValue.invalid.json";
  const char*     outFile = "ngsi10.subscribeContextResponse.circleInvertedBadValue.ok.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleZeroRadiusJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationCircleZeroRadiusJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.circleZeroRadius.postponed.json";
  const char*     outFile = "ngsi10.subscribeContextResponse.circleZeroRadius.valid.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOkJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonOkJson)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonOk.postponed.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonInvertedJson)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonInverted.postponed.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedBadValueJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonInvertedBadValueJson)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonInvertedBadValue.invalid.json";
  const char*     outFile = "ngsi10.subscribeContextResponse.polygonInvertedBadValue.valid.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonNoVerticesJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonNoVerticesJson)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonInvertedNoVertices.postponed.json";
  const char*     outFile = "ngsi10.subscribeContextResponse.polygonInvertedNoVertices.valid.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOneVertexJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonOneVertexJson)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonInvertedOneVertex.postponed.json";
  const char*     outFile = "ngsi10.subscribeContextResponse.polygonInvertedOneVertex.valid.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonTwoVerticesJson -
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonTwoVerticesJson)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonTwoVertices.postponed.json";
  const char*     outFile = "ngsi10.subscribeContextResponse.polygonTwoVertices.valid.json";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}
