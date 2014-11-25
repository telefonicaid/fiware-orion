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
#include "rest/ConnectionInfo.h"
#include "jsonParse/jsonRequest.h"
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"

#include "unittest.h"



/* ****************************************************************************
*
* Tests
* - badLength_xml
* - badLength_json
* - invalidDuration_json
*
*/



/* ****************************************************************************
*
* badLength_xml - 
*/
TEST(UpdateContextSubscriptionRequest, badLength_xml)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.updateContextSubscription.subscriptionIdLength.invalid.xml";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.subscriptionIdLengthInvalid.valid.xml";
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  lmTraceLevelSet(LmtDump, true);
  out = xmlTreat(testBuf, &ci, &parseData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_STREQ(expectedBuf, out.c_str());


  //
  // With the data obtained, render, present and release methods are exercised
  //
  UpdateContextSubscriptionRequest*  ucsrP = &parseData.ucsr.res;
  
  ucsrP->present(""); // No output

  const char*     outfile2 = "ngsi10.updateContextSubscriptionResponse.ok.valid.xml";
  const char*     outfile3 = "ngsi10.updateContextSubscriptionResponse.forcedError.valid.xml";
  const char*     outfile4 = "ngsi10.updateContextSubscriptionResponse.badDuration.valid.xml";

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  out = ucsrP->render(UpdateContextSubscription, XML, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  out  = ucsrP->check(UpdateContextSubscription, XML, "", "FORCED ERROR", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile4)) << "Error getting test data from '" << outfile4 << "'";
  ucsrP->duration.set("XXXYYYZZZ");
  out  = ucsrP->check(UpdateContextSubscription, XML, "", "", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucsrP->present("");
  ucsrP->release();

  utExit();
}



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
  const char*     outfile2 = "ngsi10.updateContextSubscriptionRequest.badLength.expected2.valid.json";
  const char*     outfile3 = "ngsi10.updateContextSubscriptionRequest.badLength.expected3.valid.json";
  const char*     outfile4 = "ngsi10.updateContextSubscriptionRequest.badLength.expected4.valid.json";
  
  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  lmTraceLevelSet(LmtDump, true);
  out = jsonTreat(testBuf, &ci, &parseData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  lmTraceLevelSet(LmtDump, false);
  EXPECT_STREQ(expectedBuf, out.c_str());

  //
  // With the data obtained, render, present and release methods are exercised
  //
  UpdateContextSubscriptionRequest*  ucsrP = &parseData.ucsr.res;
  
  ucsrP->present(""); // No output

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  out = ucsrP->render(UpdateContextSubscription, JSON, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  out  = ucsrP->check(UpdateContextSubscription, JSON, "", "FORCED ERROR", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucsrP->duration.set("XXXYYYZZZ");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile4)) << "Error getting test data from '" << outfile4 << "'";
  out  = ucsrP->check(UpdateContextSubscription, JSON, "", "", 0);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ucsrP->present("");
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  std::string out = jsonTreat(testBuf, &ci, &parseData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleOk - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationCircleOk)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.circleOk.postponed.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     result;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInverted - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationCircleInverted)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.circleInverted.postponed.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  out = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedBadValue - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationCircleInvertedBadValue)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.circleInvertedBadValue.postponed.xml";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.circleInvertedBadValue.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleZeroRadius - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationCircleZeroRadius)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.circleZeroRadius.postponed.xml";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.circleZeroRadius.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOk - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonOk)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonOk.postponed.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     result;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInverted - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonInverted)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonInverted.postponed.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     result;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedBadValue - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonInvertedBadValue)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonInvertedBadValue.postponed.xml";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.polygonInvertedBadValue.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonNoVertices - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonNoVertices)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonNoVertices.postponed.xml";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.polygonNoVertices.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOneVertex - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonOneVertex)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonOneVertex.postponed.xml";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.polygonOneVertex.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonTwoVertices - 
*/
TEST(UpdateContextSubscriptionRequest, scopeGeolocationPolygonTwoVertices)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextSubscriptionRequest.polygonTwoVertices.postponed.xml";
  const char*     outfile = "ngsi10.updateContextSubscriptionResponse.polygonTwoVertices.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContextSubscription", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, UpdateContextSubscription, "updateContextSubscriptionRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
