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

#include "common/globals.h"
#include "jsonParse/jsonRequest.h"
#include "ngsi/ParseData.h"

#include "unittest.h"



/* ****************************************************************************
*
* Tests
*   xml_ok
*   json_ok
*   noContextRegistrationList
*   json_noContextRegistration
*   emptyContextRegistration
*   noProvidingApplication
*   json_noProvidingApplication
*   emptyProvidingApplication
*   json_emptyProvidingApplication
*   noEntityIdList
*   emptyEntityIdList
*   entityIdWithEmptyId
*   entityIdWithNoId
*   entityIdWithIsPatternTrue
*   json_entityIdWithIsPatternTrue
*   present
*   invalidIsPatternString
*   json_invalidIsPatternString
*   invalidAttributeName
*   overwriteEntityIdType
*   json_overwriteEntityIdType
*   durationError
*   emptyContextRegistrationAttributeName
*   emptyContextRegistrationAttributeIsDomain
*   badContextRegistrationAttributeIsDomain
*   json_badContextRegistrationAttributeIsDomain
*   emptyContextMetadataName
*   emptyContextMetadataValue
*   emptyRegistrationMetadataValue
*
*/



/* ****************************************************************************
*
* json_ok -
*/
TEST(RegisterContextRequest, json_ok)
{
  ParseData                parseData;
  const char*              inFile   = "ngsi9.registerContextRequest.ok.valid.json";
  const char*              outFile  = "ngsi9.registerContextRequestRendered.ok.valid.json";
  RegisterContextRequest*  rcrP     = &parseData.rcr.res;
  ConnectionInfo           ci("", "POST", "1.1");
  JsonRequest*             reqP;
  std::string              out;

  ci.inMimeType      = JSON;
  ci.outMimeType     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, &reqP);
  EXPECT_EQ("OK", result) << "this test should be OK";

  out = rcrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());

  reqP->release(&parseData);
}



/* ****************************************************************************
*
* json_noContextRegistration -
*/
TEST(RegisterContextRequest, json_noContextRegistration)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.noContextRegistration.invalid.json";
  const char*     outFile = "ngsi9.registerContextResponse.noContextRegistration.valid.json";

  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* json_noProvidingApplication -
*/
TEST(RegisterContextRequest, json_noProvidingApplication)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.noProvidingApplication.invalid.json";
  const char*     outFile = "ngsi9.registerContextResponse.noProvidingApplication.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* json_emptyProvidingApplication -
*/
TEST(RegisterContextRequest, json_emptyProvidingApplication)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyProvidingApplication.invalid.json";
  const char*     outFile = "ngsi9.registerContextResponse.emptyProvidingApplication.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* json_entityIdWithIsPatternTrue -
*/
TEST(RegisterContextRequest, json_entityIdWithIsPatternTrue)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.entityIdWithIsPatternTrue.valid.json";
  const char*     outFile = "ngsi9.registerContextResponse.entityIdWithIsPatternTrue.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType   = JSON;
  ci.outMimeType  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, NULL);
  EXPECT_STREQ(expectedBuf, result.c_str()) << "entityIdWithIsPatternTrue error";
}



/* ****************************************************************************
*
* json_invalidIsPatternString -
*/
TEST(RegisterContextRequest, json_invalidIsPatternString)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.invalidIsPatternString.invalid.json";
  const char*     expect = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType   = JSON;
  ci.outMimeType  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* json_overwriteEntityIdType -
*/
TEST(RegisterContextRequest, json_overwriteEntityIdType)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.overwriteEntityIdType.invalid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType  = JSON;
  ci.outMimeType = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, NULL);
  EXPECT_EQ("OK", result) << "error at overwriting EntityIdType";
}



/* ****************************************************************************
*
* json_badContextRegistrationAttributeIsDomain -
*/
TEST(RegisterContextRequest, json_badContextRegistrationAttributeIsDomain)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.badContextRegistrationAttributeIsDomain.invalid.json";
  const char*     outFile = "ngsi9.registerContextResponse.badContextRegistrationAttributeIsDomain.valid.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType   = JSON;
  ci.outMimeType  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* json_reregistration -
*/
TEST(RegisterContextRequest, json_reregistration)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.reregistration.valid.json";
  const char*     expect = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType   = JSON;
  ci.outMimeType  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, NULL);
  EXPECT_EQ(expect, result);
}
