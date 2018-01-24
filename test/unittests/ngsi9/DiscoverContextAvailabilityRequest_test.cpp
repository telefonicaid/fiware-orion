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
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "common/globals.h"
#include "jsonParse/jsonRequest.h"

#include "unittest.h"



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
* ok_json -
*/
TEST(DiscoverContextAvailabilityRequest, ok_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.ok2.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";
  lmTraceLevelSet(LmtDump, false);
}



/* ****************************************************************************
*
* okNoRestrictions_json -
*/
TEST(DiscoverContextAvailabilityRequest, okNoRestrictions_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.noRestrictions.ok.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result) << "OK with no Restriction";
}



/* ****************************************************************************
*
* noEntityIdList_json -
*/
TEST(DiscoverContextAvailabilityRequest, noEntityIdList_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.noEntityIdList.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityRequest.noEntityIdListResponse.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* emptyEntityIdList_json -
*/
TEST(DiscoverContextAvailabilityRequest, emptyEntityIdList_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.emptyEntityIdList.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityRequest.emptyEntityIdListResponse.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* invalidIsPatternValue_json -
*/
TEST(DiscoverContextAvailabilityRequest, invalidIsPatternValue_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.isPatternValue.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityRequest.isPatternValueResponse.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* unsupportedAttributeForEntityId_json -
*/
TEST(DiscoverContextAvailabilityRequest, unsupportedAttributeForEntityId_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.unsupportedAttributeForEntityId.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.unsupportedAttributeForEntityId.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* twoEntityIdIds_json -
*/
TEST(DiscoverContextAvailabilityRequest, twoEntityIdIds_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.twoEntityIds.invalid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* entityIdTwoTypes_json -
*/
TEST(DiscoverContextAvailabilityRequest, entityIdTwoTypes_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.entityIdTwoTypes.invalid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* entityIdTwoIsPatterns_json -
*/
TEST(DiscoverContextAvailabilityRequest, entityIdTwoIsPatterns_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.entityIdTwoIsPatterns.invalid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* twoEntityIdTypes_json -
*/
TEST(DiscoverContextAvailabilityRequest, twoEntityIdTypes_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.twoEntityIdTypes.invalid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ("OK", result) << "invalid 'isPattern' value";
}



/* ****************************************************************************
*
* overrideEntityIdIsPattern_json -
*/
TEST(DiscoverContextAvailabilityRequest, overrideEntityIdIsPattern_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.overrideEntityIdIsPattern.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.overrideEntityIdIsPattern.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* emptyEntityIdId_json -
*/
TEST(DiscoverContextAvailabilityRequest, emptyEntityIdId_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.emptyEntityIdId.valid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.emptyEntityIdId.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* noEntityIdId_json -
*/
TEST(DiscoverContextAvailabilityRequest, noEntityIdId_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.noEntityIdId.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.noEntityIdId.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* noAttributeExpression_json -
*/
TEST(DiscoverContextAvailabilityRequest, noAttributeExpression_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.noAttributeExpression.invalid.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "noAttributeExpression";
}



/* ****************************************************************************
*
* emptyAttributeExpression_json -
*/
TEST(DiscoverContextAvailabilityRequest, emptyAttributeExpression_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.emptyAttributeExpression.invalid.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);

  EXPECT_EQ(expect, result) << "Empty Attribute Expression";
}



/* ****************************************************************************
*
* noScopeType_json -
*/
TEST(DiscoverContextAvailabilityRequest, noScopeType_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.noScopeType.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.noScopeType.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* noScopeValue_json -
*/
TEST(DiscoverContextAvailabilityRequest, noScopeValue_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.noScopeValue.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.noScopeValue.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* emptyScopeType_json -
*/
TEST(DiscoverContextAvailabilityRequest, emptyScopeType_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.emptyScopeType.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.emptyScopeType.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* emptyScopeValue_json -
*/
TEST(DiscoverContextAvailabilityRequest, emptyScopeValue_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.emptyScopeValue.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.emptyScopeValue.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* parseError_json -
*/
TEST(DiscoverContextAvailabilityRequest, parseError_json)
{
  ParseData       reqData;
  const char*     inFile = "ngsi9.discoverContextAvailabilityRequest.parseError.invalid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_NE("OK", result) << "Parse Error not detected";
}



/* ****************************************************************************
*
* emptyAttributeName_json -
*/
TEST(DiscoverContextAvailabilityRequest, emptyAttributeName_json)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi9.discoverContextAvailabilityRequest.emptyAttributeName.invalid.json";
  const char*     outFile = "ngsi9.discoverContextAvailabilityResponse.emptyAttributeName.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, DiscoverContextAvailability, "discoverContextAvailabilityRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}
