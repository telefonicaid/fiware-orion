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

#include "testDataFromFile.h"



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
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.ok.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";

  RegisterContextRequest*  rcrP = &reqData.rcr.res;
  std::string              out;
  std::string              expected = "<registerContextRequest>\n  <contextRegistrationList>\n    <contextRegistration>\n      <entityIdList>\n        <entityId type=\"Room\" isPattern=\"false\">\n          <id>ConferenceRoom</id>\n        </entityId>\n        <entityId type=\"Room\" isPattern=\"false\">\n          <id>OfficeRoom</id>\n        </entityId>\n      </entityIdList>\n      <contextRegistrationAttributeList>\n        <contextRegistrationAttribute>\n          <name>temperature</name>\n          <type>degree</type>\n          <isDomain>false</isDomain>\n          <registrationMetadata>\n            <contextMetadata>\n              <name>ID</name>\n              <type>string</type>\n              <value>1110</value>\n            </contextMetadata>\n            <contextMetadata>\n              <name>cm2</name>\n              <type>string</type>\n              <value>XXX</value>\n            </contextMetadata>\n          </registrationMetadata>\n        </contextRegistrationAttribute>\n      </contextRegistrationAttributeList>\n      <registrationMetadata>\n        <contextMetadata>\n          <name>ID</name>\n          <type>string</type>\n          <value>2212</value>\n        </contextMetadata>\n        <contextMetadata>\n          <name>ID2</name>\n          <type>string</type>\n          <value>212</value>\n        </contextMetadata>\n      </registrationMetadata>\n      <providingApplication>http://localhost:1028/application</providingApplication>\n    </contextRegistration>\n  </contextRegistrationList>\n  <duration>PT1H</duration>\n</registerContextRequest>\n";

  out = rcrP->render(RegisterContext, XML, "");
  EXPECT_EQ(expected, out);
}



/* ****************************************************************************
*
* json_ok - 
*/
TEST(RegisterContextRequest, json_ok)
{
  ParseData       parseData;
  const char*     fileName = "registerContextRequest_ok.json";
  ConnectionInfo  ci("", "POST", "1.1");
  JsonRequest*    reqP;

  ci.inFormat      = JSON;
  ci.outFormat     = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", &reqP);
  reqP->release(&parseData);
  EXPECT_EQ("OK", result) << "this test should be OK";
}



/* ****************************************************************************
*
* noContextRegistrationList - 
*/
TEST(RegisterContextRequest, noContextRegistrationList)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.noContextRegistration.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty Context Registration List</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_STREQ(expect, result.c_str()) << "error in no context registration list test";
}



/* ****************************************************************************
*
* json_noContextRegistration - 
*/
TEST(RegisterContextRequest, json_noContextRegistration)
{
  ParseData       reqData;
  const char*     fileName = "registerContextRequest_noContextRegistration.json";
  const char*     expect   = "{\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"Empty Context Registration List\"\n  }\n}\n";

  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "error in no context registration list test";
}



/* ****************************************************************************
*
* emptyContextRegistration - 
*/
TEST(RegisterContextRequest, emptyContextRegistration)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.emptyContextRegistration.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Empty Context Registration List</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "error in empty context registration list test";
}



/* ****************************************************************************
*
* noProvidingApplication - 
*/
TEST(RegisterContextRequest, noProvidingApplication)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.noProvidingApplication.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>no providing application</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "No Providing Application error";
}



/* ****************************************************************************
*
* json_noProvidingApplication - 
*/
TEST(RegisterContextRequest, json_noProvidingApplication)
{
  ParseData       reqData;
  const char*     fileName = "registerContextRequest_noProvidingApplication.json";
  const char*     expect   = "{\n  \"duration\" : \"PT1M\",\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"no providing application\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "No Providing Application error";
}



/* ****************************************************************************
*
* emptyProvidingApplication - 
*/
TEST(RegisterContextRequest, emptyProvidingApplication)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.emptyProvidingApplication.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>no providing application</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "Empty Providing Application error";
}



/* ****************************************************************************
*
* json_emptyProvidingApplication - 
*/
TEST(RegisterContextRequest, json_emptyProvidingApplication)
{
  ParseData       reqData;
  const char*     fileName = "registerContextRequest_emptyProvidingApplication.json";
  const char*     expect   = "{\n  \"duration\" : \"PT1M\",\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"no providing application\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "Empty Providing Application error";
}



/* ****************************************************************************
*
* noEntityIdList - 
*/
TEST(RegisterContextRequest, noEntityIdList)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.noEntityIdList.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "No EntityId List error";
}



/* ****************************************************************************
*
* emptyEntityIdList - 
*/
TEST(RegisterContextRequest, emptyEntityIdList)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.emptyEntityIdList.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "No EntityId List error";
}



/* ****************************************************************************
*
* entityIdWithEmptyId - 
*/
TEST(RegisterContextRequest, entityIdWithEmptyId)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.entityIdWithEmptyId.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>empty entityId:id</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithEmptyId error";
}



/* ****************************************************************************
*
* entityIdWithNoId - 
*/
TEST(RegisterContextRequest, entityIdWithNoId)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.entityIdWithNoId.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>empty entityId:id</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithNoId error";
}



/* ****************************************************************************
*
* entityIdWithIsPatternTrue - 
*/
TEST(RegisterContextRequest, entityIdWithIsPatternTrue)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.entityIdWithIsPatternTrue.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>'isPattern' set to true for a registration</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithIsPatternTrue error";
}



/* ****************************************************************************
*
* json_entityIdWithIsPatternTrue - 
*/
TEST(RegisterContextRequest, json_entityIdWithIsPatternTrue)
{
  ParseData       reqData;
  const char*     fileName = "registerContextRequest_entityIdWithIsPatternTrue.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat   = JSON;
  ci.outFormat  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "entityIdWithIsPatternTrue error";
}



/* ****************************************************************************
*
* present - 
*/
TEST(RegisterContextRequest, present)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.ok.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "this test should be OK";
  lmTraceLevelSet(LmtDump, false);
}



/* ****************************************************************************
*
* invalidIsPatternString - 
*/
TEST(RegisterContextRequest, invalidIsPatternString)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.isPattern.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad value for 'isPattern'</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* json_invalidIsPatternString - 
*/
TEST(RegisterContextRequest, json_invalidIsPatternString)
{
  ParseData       reqData;
  const char*     fileName = "registerContextRequest_invalidIsPatternString.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat   = JSON;
  ci.outFormat  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* invalidAttributeName - 
*/
TEST(RegisterContextRequest, invalidAttributeName)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.entityIdAttribute.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "error in invalidAttributeName";
}



/* ****************************************************************************
*
* overwriteEntityIdType - 
*/
TEST(RegisterContextRequest, overwriteEntityIdType)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.idAsEntityIdAttribute.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "error at overwriting EntityIdType";
}



/* ****************************************************************************
*
* json_overwriteEntityIdType - 
*/
TEST(RegisterContextRequest, json_overwriteEntityIdType)
{
  ParseData       reqData;
  const char*     fileName = "registerContextRequest_overwriteEntityIdType.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ("OK", result) << "error at overwriting EntityIdType";
}



/* ****************************************************************************
*
* durationError - 
*/
TEST(RegisterContextRequest, durationError)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.duration.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>invalid duration</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>syntax error in duration string</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "bad duration string was accepted as good";
}



/* ****************************************************************************
*
* emptyContextRegistrationAttributeName - 
*/
TEST(RegisterContextRequest, emptyContextRegistrationAttributeName)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.emptyContextRegistrationAttributeName.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing name for registration attribute</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Attribute Name was accepted as good";
}



/* ****************************************************************************
*
* emptyContextRegistrationAttributeIsDomain - 
*/
TEST(RegisterContextRequest, emptyContextRegistrationAttributeIsDomain)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.emptyContextRegistrationAttributeIsDomain.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing isDomain value for registration attribute</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Attribute IsDomain was accepted as good";
}



/* ****************************************************************************
*
* badContextRegistrationAttributeIsDomain - 
*/
TEST(RegisterContextRequest, badContextRegistrationAttributeIsDomain)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.contextRegistrationAttributeIsDomain.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad isDomain value for registration attribute</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "bad Context Registration Attribute IsDomain was accepted as good";
}



/* ****************************************************************************
*
* json_badContextRegistrationAttributeIsDomain - 
*/
TEST(RegisterContextRequest, json_badContextRegistrationAttributeIsDomain)
{
  ParseData       reqData;
  const char*     fileName = "registerContextRequest_badContextRegistrationAttributeIsDomain.json";
  const char*     expect   = "{\n  \"duration\" : \"PT1M\",\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"missing isDomain value for registration attribute\"\n  }\n}\n";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat   = JSON;
  ci.outFormat  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



/* ****************************************************************************
*
* emptyContextMetadataName - 
*/
TEST(RegisterContextRequest, emptyContextMetadataName)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.emptyContextMetadataName.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing metadata name</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Attribute Metadata Name was accepted as good";
}



/* ****************************************************************************
*
* emptyContextMetadataValue - 
*/
TEST(RegisterContextRequest, emptyContextMetadataValue)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.emptyContextMetadataValue.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing metadata value</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Attribute Metadata Value was accepted as good";
}



/* ****************************************************************************
*
* emptyRegistrationMetadataValue - 
*/
TEST(RegisterContextRequest, emptyRegistrationMetadataValue)
{
  ParseData       reqData;
  const char*     fileName = "ngsi9.registerContextRequest.emptyRegistrationMetadataValue.invalid.xml";
  const char*     expect   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing metadata value</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result) << "empty Context Registration Metadata Value was accepted as good";
}



/* ****************************************************************************
*
* json_reregistration - 
*/
TEST(RegisterContextRequest, json_reregistration)
{
  ParseData       reqData;
  const char*     fileName = "registerContextRequest_reregistration.json";
  const char*     expect   = "OK";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat   = JSON;
  ci.outFormat  = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = jsonTreat(testBuf, &ci, &reqData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expect, result);
}



