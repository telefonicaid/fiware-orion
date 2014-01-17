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

#include "common/globals.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/Request.h"
#include "jsonParse/jsonRequest.h"
#include "xmlParse/xmlRequest.h"
#include "xmlParse/xmlParse.h"
#include "serviceRoutines/postQueryContext.h"
#include "testDataFromFile.h"



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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  std::string out = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
}



/* ****************************************************************************
*
* entityIdType_xml - 
*/
TEST(QueryContextRequest, entityIdType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.queryContextRequest.entityIdTypeAsField.invalid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ("OK", out);
}



/* ****************************************************************************
*
* entityIdIsPattern_xml - 
*/
TEST(QueryContextRequest, entityIdIsPattern_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.queryContextRequest.entityIdIsPatternAsField.invalid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ("OK", out);
}



/* ****************************************************************************
*
* overwriteEntityIdType_xml - 
*/
TEST(QueryContextRequest, overwriteEntityIdType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.queryContextRequest.typeAsField.invalid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ("OK", out);
}



/* ****************************************************************************
*
* overwriteEntityIdIsPattern_xml - 
*/
TEST(QueryContextRequest, overwriteEntityIdIsPattern_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     infile = "ngsi10.queryContextRequest.isPatternAsField.invalid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ("OK", out);
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, out);
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, out);
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
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

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string out = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", out);
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
}
