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
#include "rest/ConnectionInfo.h"

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

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", &reqP);
  EXPECT_EQ("OK", result) << "this test should be OK";

  out = rcrP->render(RegisterContext, "");
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

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* emptyContextRegistration - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_emptyContextRegistration)
{  
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyContextRegistration.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyContextRegistration.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
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

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
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

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* noEntityIdList - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_noEntityIdList)
{
#if 0
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.noEntityIdList.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "No EntityId List error";
#endif
}



/* ****************************************************************************
*
* emptyEntityIdList - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_emptyEntityIdList)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyEntityIdList.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyEntityIdList.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
}



/* ****************************************************************************
*
* entityIdWithEmptyId - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_entityIdWithEmptyId)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.entityIdWithEmptyId.valid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.entityIdWithEmptyId.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
}



/* ****************************************************************************
*
* entityIdWithNoId - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_entityIdWithNoId)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.entityIdWithNoId.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.entityIdWithNoId.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
}



/* ****************************************************************************
*
* json_entityIdWithIsPatternTrue - 
*/
TEST(RegisterContextRequest, json_entityIdWithIsPatternTrue)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.entityIdWithIsPatternTrue.valid.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inMimeType   = JSON;
  ci.outMimeType  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithIsPatternTrue error";
}



/* ****************************************************************************
*
* present - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_present)
{
#if 0
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.ok.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";
  lmTraceLevelSet(LmtDump, false);
#endif
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

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* invalidAttributeName - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_invalidAttributeName)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.entityIdAttribute.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.entityIdAttribute.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
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

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "error at overwriting EntityIdType";
}



/* ****************************************************************************
*
* durationError - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_durationError)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.duration.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.duration.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
}



/* ****************************************************************************
*
* emptyContextRegistrationAttributeName - 
*
* FIME P5 #1862: _json counterpart?
*
*/
TEST(RegisterContextRequest, DISABLED_emptyContextRegistrationAttributeName)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyContextRegistrationAttributeName.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyContextRegistrationAttributeName.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
}



/* ****************************************************************************
*
* emptyContextRegistrationAttributeIsDomain - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_emptyContextRegistrationAttributeIsDomain)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyContextRegistrationAttributeIsDomain.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyContextRegistrationAttributeIsDomain.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
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

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* emptyContextMetadataName - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_emptyContextMetadataName)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyContextMetadataName.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyContextMetadataName.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
}



/* ****************************************************************************
*
* emptyContextMetadataValue - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_emptyContextMetadataValue)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyContextMetadataValue.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyContextMetadataValue.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
}



/* ****************************************************************************
*
* emptyRegistrationMetadataValue - 
*
* FIME P5 #1862: _json counterpart?
*/
TEST(RegisterContextRequest, DISABLED_emptyRegistrationMetadataValue)
{
#if 0
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyRegistrationMetadataValue.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyRegistrationMetadataValue.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
#endif
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

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}
