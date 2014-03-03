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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "jsonParse/jsonRequest.h"
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"

#include "unittest.h"



/* ****************************************************************************
*
* ok_xml - 
*/
TEST(SubscribeContextRequest, ok_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.subscribeContextRequest.ok.valid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", result);

  //
  // With the data obtained, render, present and release methods are exercised
  //
  SubscribeContextRequest*  scrP = &reqData.scr.res;
  const char*               outfile = "ngsi10.subscribeContextRequest.rendered.valid.xml";

  scrP->present(""); // No output

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = scrP->render(SubscribeContext, XML, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  scrP->release();
}



/* ****************************************************************************
*
* ok_json - 
*/
TEST(SubscribeContextRequest, ok_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.subscribeContextRequest.ok.valid.json";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_EQ("OK", result);
  lmTraceLevelSet(LmtDump, false);


  //
  // With the data obtained, render, present and release methods are exercised
  //
  SubscribeContextRequest*  scrP    = &parseData.scr.res;
  const char*               outfile = "ngsi10.subscribeContextRequest_ok.expected.valid.json";
  
  scrP->present(""); // No output

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = scrP->render(SubscribeContext, JSON, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  scrP->release();
}



/* ****************************************************************************
*
* invalidDuration_xml - 
*/
TEST(SubscribeContextRequest, invalidDuration_xml)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.subscribeContextRequest.duration.invalid.xml";
  const char*     outfile = "ngsi10.subscribeContextResponse.invalidDuration.valid.xml";
  XmlRequest*     reqP;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string out = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", &reqP);
  lmTraceLevelSet(LmtDump, false);

  reqP->release(&parseData);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  std::string out = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  std::string out = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* invalidEntityIdAttribute_xml - 
*
* FIXME P5: invalid attributes in EntityId are found but not reported
*/
TEST(SubscribeContextRequest, invalidEntityIdAttribute_xml)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.subscribeContextRequest.entityIdAttribute.invalid.xml";
  const char*     expected = "OK";
  XmlRequest*     reqP;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", &reqP);

  reqP->release(&parseData);
  EXPECT_STREQ(expected, result.c_str());
}



/* ****************************************************************************
*
* scopeGeolocationCircleOk - 
*/
TEST(SubscribeContextRequest, scopeGeolocationCircleOk)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.circleOk.postponed.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());
}



/* ****************************************************************************
*
* scopeGeolocationCircleInverted - 
*/
TEST(SubscribeContextRequest, scopeGeolocationCircleInverted)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.circleInverted.postponed.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedBadValue - 
*/
TEST(SubscribeContextRequest, scopeGeolocationCircleInvertedBadValue)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.circleInvertedBadValue.invalid.xml";
  const char*     outFile = "ngsi10.subscribeContextResponse.circleInvertedBadValue.invalid.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* scopeGeolocationCircleZeroRadius - 
*/
TEST(SubscribeContextRequest, scopeGeolocationCircleZeroRadius)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.circleZeroRadius.postponed.xml";
  const char*     outFile = "ngsi10.subscribeContextResponse.circleZeroRadius.postponed.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOk - 
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonOk)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonOk.postponed.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInverted - 
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonInverted)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonInverted.postponed.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  EXPECT_EQ("10",   parseData.scr.res.restriction.scopeVector.get(0)->polygon.vertexList[0]->longitude);
  EXPECT_EQ("20",   parseData.scr.res.restriction.scopeVector.get(0)->polygon.vertexList[0]->latitude);
  EXPECT_EQ("true", parseData.scr.res.restriction.scopeVector.get(0)->polygon.inverted);

  EXPECT_EQ(10,   parseData.scr.res.restriction.scopeVector.get(0)->polygon.vertexList[0]->_longitude());
  EXPECT_EQ(20,   parseData.scr.res.restriction.scopeVector.get(0)->polygon.vertexList[0]->_latitude());
  EXPECT_TRUE(parseData.scr.res.restriction.scopeVector.get(0)->polygon._inverted());
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedBadValue - 
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonInvertedBadValue)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonInvertedBadValue.invalid.xml";
  const char*     outFile = "ngsi10.subscribeContextResponse.polygonInvertedBadValue.valid.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* scopeGeolocationPolygonTwoVertices - 
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonNoVertices)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonInvertedNoVertices.postponed.xml";
  const char*     outFile = "ngsi10.subscribeContextResponse.polygonInvertedNoVertices.valid.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOneVertex - 
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonOneVertex)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonInvertedOneVertex.postponed.xml";
  const char*     outFile = "ngsi10.subscribeContextResponse.polygonInvertedOneVertex.valid.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* scopeGeolocationPolygonTwoVertices - 
*/
TEST(SubscribeContextRequest, scopeGeolocationPolygonTwoVertices)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi10.subscribeContextRequest.polygonTwoVertices.postponed.xml";
  const char*     outFile = "ngsi10.subscribeContextResponse.polygonTwoVertices.valid.xml";
  ConnectionInfo  ci("/ngsi10/subscribeContext", "POST", "1.1");
  std::string     result;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = xmlTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  result = jsonTreat(testBuf, &ci, &parseData, SubscribeContext, "subscribeContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}
