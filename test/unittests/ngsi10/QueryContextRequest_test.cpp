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
#include "ngsi/Request.h"
#include "jsonParse/jsonRequest.h"
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"
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
* ok_xml - 
*/
TEST(QueryContextRequest, ok_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.queryContextRequest.ok.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", out) << "this test should be OK";

  //
  // With the data obtained, render, present and release methods are exercised
  //
  QueryContextRequest*  qcrP = &reqData.qcr.res;
  const char*  outfile = "ngsi10.queryContextRequest.ok2.valid.xml";
  
  qcrP->present(""); // No output

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  out = qcrP->render(QueryContext, XML, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  qcrP->release();

  utExit();
}

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  lmTraceLevelSet(LmtDump, true);
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", out) << "this test should be OK";


  //
  // With the data obtained, render, present and release methods are exercised
  //
  QueryContextRequest*  qcrP = &parseData.qcr.res;
  
  qcrP->present(""); // No output

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  rendered = qcrP->render(QueryContext, JSON, "");
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  qcrP->present("");
  qcrP->release();

  utExit();
}



/* ****************************************************************************
*
* entityIdIdAsAttribute_xml - 
*/
TEST(QueryContextRequest, entityIdIdAsAttribute_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile   = "ngsi10.queryContextRequest.entityIdIdAsAttribute.invalid.xml";
  const char*     outfile  = "ngsi10.queryContextResponse.entityIdIdAsAttribute.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* badIsPattern_xml - 
*/
TEST(QueryContextRequest, badIsPattern_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.isPattern.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.isPattern.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* unsupportedEntityIdAttribute_xml - 
*/
TEST(QueryContextRequest, unsupportedEntityIdAttribute_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.attributeUnknown.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.attributeUnknown.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* entityIdType_xml - 
*/
TEST(QueryContextRequest, entityIdType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     inFile  = "ngsi10.queryContextRequest.entityIdTypeAsField.invalid.xml";
  const char*     outFile = "ngsi10.queryContextResponse.entityIdTypeAsField.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* entityIdIsPattern_xml - 
*/
TEST(QueryContextRequest, entityIdIsPattern_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     inFile  = "ngsi10.queryContextRequest.entityIdIsPatternAsField.invalid.xml";
  const char*     outFile = "ngsi10.queryContextResponse.entityIdIsPatternAsField.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* overwriteEntityIdType_xml - 
*/
TEST(QueryContextRequest, overwriteEntityIdType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     inFile  = "ngsi10.queryContextRequest.typeAsField.invalid.xml";
  const char*     outFile = "ngsi10.queryContextResponse.typeAsField.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* overwriteEntityIdIsPattern_xml - 
*/
TEST(QueryContextRequest, overwriteEntityIdIsPattern_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     inFile  = "ngsi10.queryContextRequest.isPatternAsField.invalid.xml";
  const char*     outFile = "ngsi10.queryContextResponse.isPatternAsField.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* noEntityList_xml - 
*/
TEST(QueryContextRequest, noEntityList_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.noEntityList.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.noEntityList.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* emptyEntityList_xml - 
*/
TEST(QueryContextRequest, emptyEntityList_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.emptyEntityList.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.emptyEntityList.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* emptyEntityIdId_xml - 
*/
TEST(QueryContextRequest, emptyEntityIdId_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.emptyEntityIdId.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.emptyEntityIdId.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* noAttributeExpression_xml - 
*/
TEST(QueryContextRequest, noAttributeExpression_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.queryContextRequest.noAttributeExpression.invalid.xml";
  const char*     expect = "OK";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, out);

  utExit();
}



/* ****************************************************************************
*
* emptyAttributeExpression_xml - 
*/
TEST(QueryContextRequest, emptyAttributeExpression_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.queryContextRequest.emptyAttributeExpression.invalid.xml";
  const char*     expect = "OK";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, out);

  utExit();
}



/* ****************************************************************************
*
* emptyScopeType_xml - 
*/
TEST(QueryContextRequest, emptyScopeType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.emptyScopeType.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.emptyScopeType.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* emptyScopeValue_xml - 
*/
TEST(QueryContextRequest, emptyScopeValue_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.emptyScopeValue.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.emptyScopeValue.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* noScopeType_xml - 
*/
TEST(QueryContextRequest, noScopeType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.noScopeType.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.noScopeType.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* noScopeValue_xml - 
*/
TEST(QueryContextRequest, noScopeValue_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile  = "ngsi10.queryContextRequest.noScopeValue.invalid.xml";
  const char*     outfile = "ngsi10.queryContextResponse.noScopeValue.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* noRestriction_xml - 
*/
TEST(QueryContextRequest, noRestriction_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.queryContextRequest.noRestriction.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", out);

  utExit();
}



/* ****************************************************************************
*
* fill - 
*/
TEST(QueryContextRequest, fill)
{
  QueryContextRequest q0;
  QueryContextRequest q1;
  std::string         out;
  const char*         outfile1  = "ngsi10.queryContextRequest.fill1.valid.xml";
  const char*         outfile2  = "ngsi10.queryContextRequest.fill2.valid.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  q0.fill("", "", "");
  q0.restrictions = 0;
  out = q0.render(QueryContext, XML, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  q1.fill("EID", "ETYPE", "Attribute");
  q1.restrictions = 0;
  out = q1.render(QueryContext, XML, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleOk - 
*/
TEST(QueryContextRequest, scopeGeolocationCircleOk)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.circleOk.postponed.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     result;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInverted - 
*/
TEST(QueryContextRequest, scopeGeolocationCircleInverted)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.circleInverted.postponed.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleInvertedBadValue - 
*/
TEST(QueryContextRequest, scopeGeolocationCircleInvertedBadValue)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.circleInvertedBadValue.postponed.xml";
  const char*     outfile = "ngsi10.queryContextResponse.circleInvertedBadValue.valid.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationCircleZeroRadius - 
*/
TEST(QueryContextRequest, scopeGeolocationCircleZeroRadius)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.circleZeroRadius.postponed.xml";
  const char*     outfile = "ngsi10.queryContextResponse.circleZeroRadius.valid.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  out = jsonTreat(testBuf, &ci, &reqData, SubscribeContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOk - 
*/
TEST(QueryContextRequest, scopeGeolocationPolygonOk)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonOk.postponed.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     result;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInverted - 
*/
TEST(QueryContextRequest, scopeGeolocationPolygonInverted)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonInverted.postponed.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     result;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonInvertedBadValue - 
*/
TEST(QueryContextRequest, scopeGeolocationPolygonInvertedBadValue)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonInvertedBadValue.postponed.xml";
  const char*     outfile = "ngsi10.queryContextResponse.polygonInvertedBadValue.valid.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonNoVertices - 
*/
TEST(QueryContextRequest, scopeGeolocationPolygonNoVertices)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonNoVertices.postponed.xml";
  const char*     outfile = "ngsi10.queryContextResponse.polygonNoVertices.valid.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonOneVertex - 
*/
TEST(QueryContextRequest, scopeGeolocationPolygonOneVertex)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonOneVertex.postponed.xml";
  const char*     outfile = "ngsi10.queryContextResponse.polygonOneVertex.valid.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* scopeGeolocationPolygonTwoVertices - 
*/
TEST(QueryContextRequest, scopeGeolocationPolygonTwoVertices)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.queryContextRequest.polygonTwoVertices.postponed.xml";
  const char*     outfile = "ngsi10.queryContextResponse.polygonTwoVertices.valid.xml";
  ConnectionInfo  ci("/ngsi10/queryContext", "POST", "1.1");
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  out = jsonTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
