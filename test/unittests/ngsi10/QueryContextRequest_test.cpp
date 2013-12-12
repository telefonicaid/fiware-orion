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
  const char*     fileName = "ngsi10.queryContextRequest.ok.valid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", result) << "this test should be OK";

  //
  // With the data obtained, render, present and release methods are exercised
  //
  QueryContextRequest*  qcrP = &reqData.qcr.res;
  
  qcrP->present(""); // No output

  std::string rendered;
  std::string expected = "<queryContextRequest>\n  <entityIdList>\n    <entityId type=\"Room\" isPattern=\"false\">\n      <id>ConferenceRoom</id>\n    </entityId>\n    <entityId type=\"Room\" isPattern=\"false\">\n      <id>OfficeRoom</id>\n    </entityId>\n  </entityIdList>\n  <attributeList>\n    <attribute>temperature</attribute>\n    <attribute>occupancy</attribute>\n    <attribute>lightstatus</attribute>\n  </attributeList>\n  <restriction>\n    <attributeExpression>Attribute Expression</attributeExpression>\n    <scope>\n      <operationScope>\n        <type>st1</type>\n        <value>sv1</value>\n      </operationScope>\n      <operationScope>\n        <type>st2</type>\n        <value>sv2</value>\n      </operationScope>\n    </scope>\n  </restriction>\n</queryContextRequest>\n";

  rendered = qcrP->render(QueryContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

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
  const char*     fileName     = "queryContextRequest_ok.json";  
  const char*     expectedFile = "ngsi10.queryContextRequest_ok.expected.valid.json";
  std::string     rendered;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", result) << "this test should be OK";


  //
  // With the data obtained, render, present and release methods are exercised
  //
  QueryContextRequest*  qcrP = &parseData.qcr.res;
  
  qcrP->present(""); // No output


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), expectedFile)) << "Error getting test data from '" << expectedFile << "'";
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
  const char*     fileName = "ngsi10.queryContextRequest.entityIdIdAsAttribute.invalid.xml";
  std::string     expected = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* badIsPattern_xml - 
*/
TEST(QueryContextRequest, badIsPattern_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.isPattern.invalid.xml";
  const char*     expected = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad value for 'isPattern'</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_STREQ(expected, result.c_str());
}



/* ****************************************************************************
*
* badIsPattern_json - 
*/
TEST(QueryContextRequest, badIsPattern_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "queryContextRequest_badIsPattern.json";
  const char*     expected = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"bad value for 'isPattern'\"\n  }\n}\n";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  std::string result = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* emptyAttribute_json - 
*/
TEST(QueryContextRequest, emptyAttribute_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "queryContextRequest_emptyAttribute.json";
  const char*     expected = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"Empty attribute name\"\n  }\n}\n";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  std::string result = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* emptyAttributeExpression_json - 
*/
TEST(QueryContextRequest, emptyAttributeExpression_json)
{
  ParseData       parseData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "queryContextRequest_emptyAttributeExpression.json";
  const char*     expected = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"Empty attribute expression\"\n  }\n}\n";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  std::string result = jsonTreat(testBuf, &ci, &parseData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* unsupportedEntityIdAttribute_xml - 
*/
TEST(QueryContextRequest, unsupportedEntityIdAttribute_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.attributeUnknown.invalid.xml";
  const char*     expect   = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result) << "unsupportedEntityIdAttribute";
}



/* ****************************************************************************
*
* entityIdType_xml - 
*/
TEST(QueryContextRequest, entityIdType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.entityIdTypeAsField.invalid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ("OK", result) << "entityIdType";
}



/* ****************************************************************************
*
* entityIdIsPattern_xml - 
*/
TEST(QueryContextRequest, entityIdIsPattern_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.entityIdIsPatternAsField.invalid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ("OK", result) << "entityIdIsPattern";
}



/* ****************************************************************************
*
* overwriteEntityIdType_xml - 
*/
TEST(QueryContextRequest, overwriteEntityIdType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.typeAsField.invalid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ("OK", result) << "overwriteEntityIdType";
}



/* ****************************************************************************
*
* overwriteEntityIdIsPattern_xml - 
*/
TEST(QueryContextRequest, overwriteEntityIdIsPattern_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.isPatternAsField.invalid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ("OK", result) << "overwriteEntityIdIsPattern";
}



/* ****************************************************************************
*
* noEntityList_xml - 
*/
TEST(QueryContextRequest, noEntityList_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.noEntityList.invalid.xml";
  const char*     expect   = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>No entities</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result) << "noEntityList";
}



/* ****************************************************************************
*
* emptyEntityList_xml - 
*/
TEST(QueryContextRequest, emptyEntityList_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.emptyEntityList.invalid.xml";
  const char*     expect   = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>No entities</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result) << "emptyEntityList";
}



/* ****************************************************************************
*
* emptyEntityIdId_xml - 
*/
TEST(QueryContextRequest, emptyEntityIdId_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.emptyEntityIdId.invalid.xml";
  const char*     expect   = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>empty entityId:id</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result) << "emptyEntityIdId";
}



/* ****************************************************************************
*
* noAttributeExpression_xml - 
*/
TEST(QueryContextRequest, noAttributeExpression_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.noAttributeExpression.invalid.xml";
  const char*     expect   = "OK";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* emptyAttributeExpression_xml - 
*/
TEST(QueryContextRequest, emptyAttributeExpression_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.emptyAttributeExpression.invalid.xml";
  const char*     expect   = "OK";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* emptyScopeType_xml - 
*/
TEST(QueryContextRequest, emptyScopeType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.emptyScopeType.invalid.xml";
  const char*     expect   = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty type in restriction scope</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result) << "emptyScopeType";
}



/* ****************************************************************************
*
* emptyScopeValue_xml - 
*/
TEST(QueryContextRequest, emptyScopeValue_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.emptyScopeValue.invalid.xml";
  const char*     expect   = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty value in restriction scope</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result) << "emptyScopeValue";
}



/* ****************************************************************************
*
* noScopeType_xml - 
*/
TEST(QueryContextRequest, noScopeType_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.noScopeType.invalid.xml";
  const char*     expect   = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty type in restriction scope</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result) << "noScopeType";
}



/* ****************************************************************************
*
* noScopeValue_xml - 
*/
TEST(QueryContextRequest, noScopeValue_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.noScopeValue.invalid.xml";
  const char*     expect   = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty value in restriction scope</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  EXPECT_EQ(expect, result) << "noScopeValue";
}



/* ****************************************************************************
*
* noRestriction_xml - 
*/
TEST(QueryContextRequest, noRestriction_xml)
{
  ParseData       reqData;
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     fileName = "ngsi10.queryContextRequest.noRestriction.valid.xml";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, QueryContext, "queryContextRequest", NULL);
  lmTraceLevelSet(LmtDump, false);

  EXPECT_EQ("OK", result) << "noRestriction";
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
  std::string         expected0 = "<queryContextRequest>\n  <entityIdList>\n    <entityId type=\"\" isPattern=\"true\">\n      <id></id>\n    </entityId>\n  </entityIdList>\n</queryContextRequest>\n";
  std::string         expected1 = "<queryContextRequest>\n  <entityIdList>\n    <entityId type=\"ETYPE\" isPattern=\"true\">\n      <id>EID</id>\n    </entityId>\n  </entityIdList>\n  <attributeList>\n    <attribute>Attribute</attribute>\n  </attributeList>\n</queryContextRequest>\n";

  q0.fill("", "", "");
  q0.restrictions = 0;
  out = q0.render(QueryContext, XML, "");
  EXPECT_EQ(expected0, out);

  q1.fill("EID", "ETYPE", "Attribute");
  q1.restrictions = 0;
  out = q1.render(QueryContext, XML, "");
  EXPECT_EQ(expected1, out);
}
