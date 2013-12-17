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
#include "jsonParse/jsonRequest.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/xmlRequest.h"

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
* xml_ok - 
*/
TEST(RegisterContextRequest, xml_ok)
{
  ParseData                parseData;
  const char*              inFile  = "ngsi9.registerContextRequest.ok.valid.xml";
  const char*              outFile = "ngsi9.registerContextRequestRendered.ok.valid.xml";
  RegisterContextRequest*  rcrP    = &parseData.rcr.res;
  ConnectionInfo           ci("", "POST", "1.1");
  std::string              out;
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";

  out = rcrP->render(RegisterContext, XML, "");
  EXPECT_STREQ(expectedBuf, out.c_str());
}



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

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", &reqP);
  EXPECT_EQ("OK", result) << "this test should be OK";

  out = rcrP->render(RegisterContext, JSON, "");
  EXPECT_STREQ(expectedBuf, out.c_str());

  reqP->release(&parseData);
}



/* ****************************************************************************
*
* noContextRegistrationList - 
*/
TEST(RegisterContextRequest, noContextRegistrationList)
{
  ParseData       parseData;
  const char*     inFile   = "ngsi9.registerContextRequest.noContextRegistration.invalid.xml";
  const char*     outFile  = "ngsi9.registerContextResponse.noContextRegistration.valid.xml";
  
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* emptyContextRegistration - 
*/
TEST(RegisterContextRequest, emptyContextRegistration)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyContextRegistration.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyContextRegistration.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* noProvidingApplication - 
*/
TEST(RegisterContextRequest, noProvidingApplication)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.noProvidingApplication.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.noProvidingApplication.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* emptyProvidingApplication - 
*/
TEST(RegisterContextRequest, emptyProvidingApplication)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyProvidingApplication.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyProvidingApplication.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
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

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* noEntityIdList - 
*/
TEST(RegisterContextRequest, noEntityIdList)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.noEntityIdList.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "No EntityId List error";
}



/* ****************************************************************************
*
* emptyEntityIdList - 
*/
TEST(RegisterContextRequest, emptyEntityIdList)
{
  ParseData       parseData;
  const char*     inFile  = "ngsi9.registerContextRequest.emptyEntityIdList.invalid.xml";
  const char*     outFile = "ngsi9.registerContextResponse.emptyEntityIdList.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, result.c_str());
}



/* ****************************************************************************
*
* entityIdWithEmptyId - 
*/
TEST(RegisterContextRequest, entityIdWithEmptyId)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.entityIdWithEmptyId.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>empty entityId:id</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithEmptyId error";
}



/* ****************************************************************************
*
* entityIdWithNoId - 
*/
TEST(RegisterContextRequest, entityIdWithNoId)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.entityIdWithNoId.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>empty entityId:id</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithNoId error";
}



/* ****************************************************************************
*
* entityIdWithIsPatternTrue - 
*/
TEST(RegisterContextRequest, entityIdWithIsPatternTrue)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.entityIdWithIsPatternTrue.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>'isPattern' set to true for a registration</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithIsPatternTrue error";
}



/* ****************************************************************************
*
* json_entityIdWithIsPatternTrue - 
*/
TEST(RegisterContextRequest, json_entityIdWithIsPatternTrue)
{
  ParseData       parseData;
  const char*     inFile = "registerContextRequest_entityIdWithIsPatternTrue.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat   = JSON;
  ci.outFormat  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithIsPatternTrue error";
}



/* ****************************************************************************
*
* present - 
*/
TEST(RegisterContextRequest, present)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.ok.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";
  lmTraceLevelSet(LmtDump, false);
}



/* ****************************************************************************
*
* invalidIsPatternString - 
*/
TEST(RegisterContextRequest, invalidIsPatternString)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.isPattern.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad value for 'isPattern'</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* json_invalidIsPatternString - 
*/
TEST(RegisterContextRequest, json_invalidIsPatternString)
{
  ParseData       parseData;
  const char*     inFile = "registerContextRequest_invalidIsPatternString.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat   = JSON;
  ci.outFormat  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* invalidAttributeName - 
*/
TEST(RegisterContextRequest, invalidAttributeName)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.entityIdAttribute.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "error in invalidAttributeName";
}



/* ****************************************************************************
*
* json_overwriteEntityIdType - 
*/
TEST(RegisterContextRequest, json_overwriteEntityIdType)
{
  ParseData       parseData;
  const char*     inFile = "registerContextRequest_overwriteEntityIdType.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "error at overwriting EntityIdType";
}



/* ****************************************************************************
*
* durationError - 
*/
TEST(RegisterContextRequest, durationError)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.duration.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>invalid duration</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>syntax error in duration string</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "bad duration string was accepted as good";
}



/* ****************************************************************************
*
* emptyContextRegistrationAttributeName - 
*/
TEST(RegisterContextRequest, emptyContextRegistrationAttributeName)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.emptyContextRegistrationAttributeName.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing name for registration attribute</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Attribute Name was accepted as good";
}



/* ****************************************************************************
*
* emptyContextRegistrationAttributeIsDomain - 
*/
TEST(RegisterContextRequest, emptyContextRegistrationAttributeIsDomain)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.emptyContextRegistrationAttributeIsDomain.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing isDomain value for registration attribute</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Attribute IsDomain was accepted as good";
}



/* ****************************************************************************
*
* badContextRegistrationAttributeIsDomain - 
*/
TEST(RegisterContextRequest, badContextRegistrationAttributeIsDomain)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.contextRegistrationAttributeIsDomain.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad isDomain value for registration attribute</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "bad Context Registration Attribute IsDomain was accepted as good";
}



/* ****************************************************************************
*
* json_badContextRegistrationAttributeIsDomain - 
*/
TEST(RegisterContextRequest, json_badContextRegistrationAttributeIsDomain)
{
  ParseData       parseData;
  const char*     inFile = "registerContextRequest_badContextRegistrationAttributeIsDomain.json";
  const char*     expect   = "{\n  \"duration\" : \"PT1M\",\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"missing isDomain value for registration attribute\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat   = JSON;
  ci.outFormat  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* emptyContextMetadataName - 
*/
TEST(RegisterContextRequest, emptyContextMetadataName)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.emptyContextMetadataName.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing metadata name</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Attribute Metadata Name was accepted as good";
}



/* ****************************************************************************
*
* emptyContextMetadataValue - 
*/
TEST(RegisterContextRequest, emptyContextMetadataValue)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.emptyContextMetadataValue.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing metadata value</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Attribute Metadata Value was accepted as good";
}



/* ****************************************************************************
*
* emptyRegistrationMetadataValue - 
*/
TEST(RegisterContextRequest, emptyRegistrationMetadataValue)
{
  ParseData       parseData;
  const char*     inFile = "ngsi9.registerContextRequest.emptyRegistrationMetadataValue.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing metadata value</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Metadata Value was accepted as good";
}



/* ****************************************************************************
*
* json_reregistration - 
*/
TEST(RegisterContextRequest, json_reregistration)
{
  ParseData       parseData;
  const char*     inFile = "registerContextRequest_reregistration.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat   = JSON;
  ci.outFormat  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



