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

#include "ngsi/ParseData.h"
#include "common/globals.h"
#include "jsonParse/jsonRequest.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/xmlRequest.h"

#include "testDataFromFile.h"



/* ****************************************************************************
*
* Tests
* - ok
* - okNoRestrictions
* - noEntityIdList
* - emptyEntityIdList
* - invalidIsPatternValue
* - unsupportedAttributeForEntityId
* - entityIdIdAsAttribute
* - entityIdType
* - entityIdIsPattern
* - overrideEntityIdType
* - overrideEntityIdIsPattern
* - emptyEntityIdId
* - noEntityIdId
* - noAttributeExpression
* - emptyAttributeExpression
* - noScopeType
* - noScopeValue
* - emptyScopeType
* - emptyScopeValue
* - emptyAttributeName
*/



/* ****************************************************************************
*
* ok_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, ok_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.ok.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";
  lmTraceLevelSet(LmtDump, false);
}



/* ****************************************************************************
*
* ok_json - 
*/
TEST(DiscoverContextAvailabilityRequest, ok_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_ok.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";
  lmTraceLevelSet(LmtDump, false);
}



/* ****************************************************************************
*
* okNoRestrictions_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, okNoRestrictions_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.noRestriction.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "OK with no Restriction";
}



/* ****************************************************************************
*
* okNoRestrictions_json - 
*/
TEST(DiscoverContextAvailabilityRequest, okNoRestrictions_json)
{
  ParseData       reqData; 
  const char*     fileName = "discoverContextAvailabilityRequest_okNoRestrictions.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "OK with no Restriction";
}



/* ****************************************************************************
*
* noEntityIdList_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, noEntityIdList_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.noEntityIdList.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>404</code>\n    <reasonPhrase>No context element found</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "No entities";
}



/* ****************************************************************************
*
* noEntityIdList_json - 
*/
TEST(DiscoverContextAvailabilityRequest, noEntityIdList_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_noEntityIdList.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"404\",\n    \"reasonPhrase\" : \"No context element found\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "No entities";
}



/* ****************************************************************************
*
* emptyEntityIdList_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyEntityIdList_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.emptyEntityIdList.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>404</code>\n    <reasonPhrase>No context element found</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "empty EntityId list";
}



/* ****************************************************************************
*
* emptyEntityIdList_json - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyEntityIdList_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_emptyEntityIdList.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"404\",\n    \"reasonPhrase\" : \"No context element found\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "empty EntityId list";
}



/* ****************************************************************************
*
* invalidIsPatternValue_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, invalidIsPatternValue_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.isPattern.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad value for 'isPattern'</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* invalidIsPatternValue_json - 
*/
TEST(DiscoverContextAvailabilityRequest, invalidIsPatternValue_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_invalidIsPatternValue.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"bad value for 'isPattern'\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  LM_W(("Buffer: '%s'", testBuf));
  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* unsupportedAttributeForEntityId_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, unsupportedAttributeForEntityId_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.unsupportedAttributeForEntityId.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "Unsupported Attribute For EntityId was NOT detected!";
}



/* ****************************************************************************
*
* unsupportedAttributeForEntityId_json - 
*/
TEST(DiscoverContextAvailabilityRequest, unsupportedAttributeForEntityId_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_unsupportedAttributeForEntityId.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "Unsupported Attribute For EntityId";
}



/* ****************************************************************************
*
* entityIdIdAsAttribute_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, entityIdIdAsAttribute_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.entityIdIdAsAttribute.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");
  std::string     expected = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* entityIdIdAsAttribute_json - 
*/
TEST(DiscoverContextAvailabilityRequest, entityIdIdAsAttribute_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_entityIdIdAsAttribute.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* entityIdType_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, entityIdType_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.entityIdTypeAsField.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* entityIdType_json - 
*/
TEST(DiscoverContextAvailabilityRequest, entityIdType_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_entityIdType.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* entityIdIsPattern_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, entityIdIsPattern_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.entityIdIsPatternAsField.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* entityIdIsPattern_json - 
*/
TEST(DiscoverContextAvailabilityRequest, entityIdIsPattern_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_entityIdIsPattern.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* overrideEntityIdId_json - 
*/
TEST(DiscoverContextAvailabilityRequest, overrideEntityIdId_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_overrideEntityIdId.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* overrideEntityIdType_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, overrideEntityIdType_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.EntityIdTypeAsField.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* overrideEntityIdType_json - 
*/
TEST(DiscoverContextAvailabilityRequest, overrideEntityIdType_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_overrideEntityIdType.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* overrideEntityIdIsPattern_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, overrideEntityIdIsPattern_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.EntityIdIsPattern.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad value for 'isPattern'</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* overrideEntityIdIsPattern_json - 
*/
TEST(DiscoverContextAvailabilityRequest, overrideEntityIdIsPattern_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_overrideEntityIdIsPattern.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"bad value for 'isPattern'\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* emptyEntityIdId_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyEntityIdId_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.emptyEntityIdId.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>empty entityId:id</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "empty EntityId id";
}



/* ****************************************************************************
*
* emptyEntityIdId_json - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyEntityIdId_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_emptyEntityIdId.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"empty entityId:id\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "empty EntityId id";
}



/* ****************************************************************************
*
* noEntityIdId_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, noEntityIdId_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.noEntityIdId.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>empty entityId:id</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "No EntityId id";
}



/* ****************************************************************************
*
* noEntityIdId_json - 
*/
TEST(DiscoverContextAvailabilityRequest, noEntityIdId_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_noEntityIdId.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"empty entityId:id\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "No EntityId id";
}



/* ****************************************************************************
*
* noAttributeExpression_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, noAttributeExpression_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.noAttributeExpression.invalid.xml";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "noAttributeExpression";
}



/* ****************************************************************************
*
* noAttributeExpression_json - 
*/
TEST(DiscoverContextAvailabilityRequest, noAttributeExpression_json)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.noAttributeExpression.invalid.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "noAttributeExpression";
}



/* ****************************************************************************
*
* emptyAttributeExpression_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyAttributeExpression_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.emptyAttributeExpression.invalid.xml";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "Empty Attribute Expression";
}



/* ****************************************************************************
*
* emptyAttributeExpression_json - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyAttributeExpression_json)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.emptyAttributeExpression.invalid.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "Empty Attribute Expression";
}



/* ****************************************************************************
*
* noScopeType_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, noScopeType_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.noScopeType.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty type in restriction scope</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "No Scope Type";
}



/* ****************************************************************************
*
* noScopeType_json - 
*/
TEST(DiscoverContextAvailabilityRequest, noScopeType_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_noScopeType.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"Empty type in restriction scope\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "No Scope Type";
}



/* ****************************************************************************
*
* noScopeValue_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, noScopeValue_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.noScopeValue.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty value in restriction scope</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "No Scope Value";
}



/* ****************************************************************************
*
* noScopeValue_json - 
*/
TEST(DiscoverContextAvailabilityRequest, noScopeValue_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_noScopeValue.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"Empty value in restriction scope\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "No Scope Value";
}



/* ****************************************************************************
*
* emptyScopeType_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyScopeType_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.emptyScopeType.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty type in restriction scope</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "Empty Scope Type";
}



/* ****************************************************************************
*
* emptyScopeType_json - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyScopeType_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_emptyScopeType.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"Empty type in restriction scope\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "Empty Scope Type";
}



/* ****************************************************************************
*
* emptyScopeValue_xml - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyScopeValue_xml)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.discoverContextAvailabilityRequest.emptyScopeValue.invalid.xml";
  const char*     expect   = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty value in restriction scope</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expect, result) << "Empty Scope Value";
}



/* ****************************************************************************
*
* emptyScopeValue_json - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyScopeValue_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_emptyScopeValue.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"Empty value in restriction scope\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "Empty Scope Value";
}



/* ****************************************************************************
*
* parseError_json - 
*/
TEST(DiscoverContextAvailabilityRequest, parseError_json)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_parseError.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_NE("OK", result) << "Parse Error not detected";
}



/* ****************************************************************************
*
* emptyAttributeName - 
*/
TEST(DiscoverContextAvailabilityRequest, emptyAttributeName)
{
  ParseData       reqData;
  const char*     fileName = "discoverContextAvailabilityRequest_emptyAttributeName.json";
  ConnectionInfo  ci("", "POST", "1.1");
  const char*     expected = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"empty attribute name\"\n  }\n}\n";

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result);
}



