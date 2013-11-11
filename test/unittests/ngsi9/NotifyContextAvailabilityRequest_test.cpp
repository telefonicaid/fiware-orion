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
* ok_xml - 
*/
TEST(NotifyContextAvailabilityRequest, ok_xml)
{
  ParseData       parseData;
  const char*     fileName = "ngsi9.notifyContextAvailabilityRequest.ok.valid.xml";
  ConnectionInfo  ci("", "POST", "1.1");

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = xmlTreat(testBuf, &ci, &parseData, NotifyContextAvailability, "notifyContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result);
  lmTraceLevelSet(LmtDump, false);

  NotifyContextAvailabilityRequest* ncarP = &parseData.ncar.res;

  std::string     rendered = ncarP->render(NotifyContext, XML, "");
  std::string     expected = "<notifyContextAvailabilityRequest>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n  <contextRegistrationResponseList>\n    <contextRegistrationResponse>\n      <contextRegistration>\n        <entityIdList>\n          <entityId type=\"Room\" isPattern=\"false\">\n            <id>ConferenceRoom</id>\n          </entityId>\n          <entityId type=\"Room\" isPattern=\"false\">\n            <id>OfficeRoom</id>\n          </entityId>\n        </entityIdList>\n        <contextRegistrationAttributeList>\n          <contextRegistrationAttribute>\n            <name>temperature</name>\n            <type>degree</type>\n            <isDomain>false</isDomain>\n            <registrationMetadata>\n              <contextMetadata>\n                <name>ID</name>\n                <type>string</type>\n                <value>1110</value>\n              </contextMetadata>\n            </registrationMetadata>\n          </contextRegistrationAttribute>\n        </contextRegistrationAttributeList>\n        <registrationMetadata>\n          <contextMetadata>\n            <name>ID</name>\n            <type>string</type>\n            <value>2212</value>\n          </contextMetadata>\n        </registrationMetadata>\n        <providingApplication>http://192.168.100.1:70/application\n\t\t\t\t</providingApplication>\n      </contextRegistration>\n    </contextRegistrationResponse>\n  </contextRegistrationResponseList>\n</notifyContextAvailabilityRequest>\n";

  EXPECT_EQ(expected, rendered);

  ncarP->release();
}



/* ****************************************************************************
*
* ok_json - 
*/
TEST(NotifyContextAvailabilityRequest, ok_json)
{
  ParseData       parseData;
  const char*     fileName = "notifyContextAvailabilityRequest_ok.json";
  ConnectionInfo  ci("", "POST", "1.1");

  ci.inFormat  = JSON;
  ci.outFormat = JSON;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  lmTraceLevelSet(LmtDump, true);
  std::string result = jsonTreat(testBuf, &ci, &parseData, NotifyContextAvailability, "notifyContextAvailabilityRequest", NULL);
  EXPECT_EQ("OK", result);
  lmTraceLevelSet(LmtDump, false);

  NotifyContextAvailabilityRequest* ncarP = &parseData.ncar.res;

  std::string     rendered = ncarP->render(NotifyContext, JSON, "");
  std::string     expected = "{\n  \"subscriptionId\" : \"012345678901234567890123\"\n  \"contextRegistrationResponses\" : [\n    {\n      \"contextRegistration\" : {\n        \"entities\" : [\n          {\n            \"type\" : \"Room\",\n            \"isPattern\" : \"false\",\n            \"id\" : \"ConferenceRoom\"\n          },\n          {\n            \"type\" : \"Room\",\n            \"isPattern\" : \"false\",\n            \"id\" : \"OfficeRoom\"\n          }\n        ],\n        \"attributes\" : [\n          {\n            \"name\" : \"temperature\",\n            \"type\" : \"degree\",\n            \"isDomain\" : \"false\",\n            \"registrationMetadata\" : {\n              \"contextMetadata\" : {\n                \"name\" : \"ID\",\n                \"type\" : \"string\",\n                \"value\" : \"1110\"\n              }\n            }\n          }\n        ],\n        \"registrationMetadata\" : {\n          \"contextMetadata\" : {\n            \"name\" : \"ID\",\n            \"type\" : \"string\",\n            \"value\" : \"2212\"\n          }\n        },\n        \"providingApplication\" : \"http://192.168.100.1:70/application\"\n      }\n    }\n  ]\n}\n";

  EXPECT_EQ(expected, rendered);

  ncarP->release();
}



/* ****************************************************************************
*
* badEntityAttribute_xml - 
*/
TEST(NotifyContextAvailabilityRequest, badEntityAttribute_xml)
{
  ParseData       parseData;
  const char*     fileName = "ngsi9.notifyContextAvailabilityRequest.entityAttribute.invalid.xml";
  ConnectionInfo  ci("", "POST", "1.1");
  std::string     expected = "<notifyContextAvailabilityResponse>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </statusCode>\n</notifyContextAvailabilityResponse>\n";

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  std::string result = xmlTreat(testBuf, &ci, &parseData, NotifyContextAvailability, "notifyContextAvailabilityRequest", NULL);
  EXPECT_EQ(expected, result);
}



/* ****************************************************************************
*
* check - 
*/
TEST(NotifyContextAvailabilityRequest, check)
{
  NotifyContextAvailabilityRequest  ncr;
  std::string                       check;
  std::string                       expected1 = "OK";
  std::string                       expected2 = "<notifyContextAvailabilityResponse>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>predetected error</reasonPhrase>\n  </statusCode>\n</notifyContextAvailabilityResponse>\n";
  std::string                       expected3 = "<notifyContextAvailabilityResponse>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>bad length (24 chars expected)</reasonPhrase>\n  </statusCode>\n</notifyContextAvailabilityResponse>\n";

  // check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
  check = ncr.check(NotifyContextAvailability, XML, "", "", 0);
  EXPECT_EQ(expected1, check);

  check = ncr.check(NotifyContextAvailability, XML, "", "predetected error", 0);
  EXPECT_EQ(expected2, check);
 
  ncr.subscriptionId.set("12345");
  check = ncr.check(NotifyContextAvailability, XML, "", "", 0);
  EXPECT_EQ(expected3, check);
}
