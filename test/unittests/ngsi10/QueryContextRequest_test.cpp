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
#include "ngsi/Request.h"
#include "jsonParse/jsonRequest.h"
#include "serviceRoutines/postQueryContext.h"

#include "unittest.h"



/* ****************************************************************************
*
* Tests
* - ok
* - entityIdIdAsAttribute
* - badIsPattern
* - unsupportedEntityIdAttribute
* - entityIdType
* - entityIdIsPattern
* - overwriteEntityIdType
* - overwriteEntityIdIsPattern
* - overwriteEntityIdId
* - noEntityList
* - emptyEntityList
* - emptyEntityIdId
* - noAttributeExpression
* - emptyAttributeExpression
* - emptyScopeType
* - emptyScopeValue
* - noScopeType
* - noScopeValue
* - noRestriction
*
*/



/* ****************************************************************************
*
* ok_json -
*/
TEST(QueryContextRequest, ok_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest_ok.valid.json";
  const char*     outfile = "ngsi10.queryContextRequest_ok.expected.valid.json";
  std::string     rendered;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  lmTraceLevelSet(LmtDump, true);
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", out) << "this test should be OK";


  //
  // With the data obtained, render, present and release methods are exercised
  //
  QueryContextRequest*  qcrP = &parseData.qcr.res;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  rendered = qcrP->render();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  qcrP->release();

  utExit();
}



/* ****************************************************************************
*
* badIsPattern_json -
*/
TEST(QueryContextRequest, badIsPattern_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.badIsPattern.invalid.json";
  const char*     outfile = "ngsi10.queryContextResponse.badIsPattern.valid.json";

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* emptyAttribute_json -
*/
TEST(QueryContextRequest, emptyAttribute_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.emptyAttribute.valid.json";
  const char*     outfile = "ngsi10.queryContextResponse.emptyAttribute.valid.json";

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* emptyAttributeExpression_json -
*/
TEST(QueryContextRequest, emptyAttributeExpression_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.emptyAttributeExpression.invalid.json";
  const char*     outfile = "ngsi10.queryContextResponse.emptyAttributeExpression.valid.json";

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleOkJson -
*/
TEST(QueryContextRequest, scopeGeolocationCircleOkJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.circleOk.postponed.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  out = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "queryContextRequest", NULL);
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedJson -
*/
TEST(QueryContextRequest, scopeGeolocationCircleInvertedJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.circleInverted.postponed.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  out = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "queryContextRequest", NULL);
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedBadValueJson -
*/
TEST(QueryContextRequest, scopeGeolocationCircleInvertedBadValueJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.circleInvertedBadValue.postponed.json";
  const char*     outFile = "ngsi10.queryContextRequest.circleInvertedBadValue.valid.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  out = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleZeroRadiusJson -
*/
TEST(QueryContextRequest, scopeGeolocationCircleZeroRadiusJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.circleZeroRadius.postponed.json";
  const char*     outFile = "ngsi10.queryContextRequest.circleZeroRadius.valid.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  out = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOkJson -
*/
TEST(QueryContextRequest, scopeGeolocationPolygonOkJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonOk.postponed.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedJson -
*/
TEST(QueryContextRequest, scopeGeolocationPolygonInvertedJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonInverted.postponed.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     result;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedBadValueJson -
*/
TEST(QueryContextRequest, scopeGeolocationPolygonInvertedBadValueJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonInvertedBadValue.postponed.json";
  const char*     outfile = "ngsi10.queryContextResponse.polygonInvertedBadValue.valid.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonNoVerticesJson -
*/
TEST(QueryContextRequest, scopeGeolocationPolygonNoVerticesJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonNoVertices.postponed.json";
  const char*     outfile = "ngsi10.queryContextResponse.polygonNoVertices.valid.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOneVertexJson -
*/
TEST(QueryContextRequest, scopeGeolocationPolygonOneVertexJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonOneVertex.postponed.json";
  const char*     outfile = "ngsi10.queryContextResponse.polygonOneVertex.valid.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonTwoVerticesJson -
*/
TEST(QueryContextRequest, scopeGeolocationPolygonTwoVerticesJson)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonTwoVertices.postponed.json";
  const char*     outfile = "ngsi10.queryContextResponse.polygonTwoVertices.valid.json";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
