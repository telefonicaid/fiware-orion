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

#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/getContextEntityTypes.h"
#include "serviceRoutines/badVerbGetPostOnly.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "testDataFromFile.h"
#include "testInit.h"
#include "commonMocks.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST RegisterContext",                     "POST", RegisterContext,                      2, { "ngsi9", "registerContext"                        }, "", postRegisterContext   },
  { "GET ContextEntityTypeAttributeContainer",  "GET",  ContextEntityTypeAttributeContainer,  4, { "ngsi9", "contextEntityTypes", "*", "attributes"  }, "", getContextEntityTypes },
  { "* ContextEntityTypeAttributeContainer",    "*",    ContextEntityTypeAttributeContainer,  4, { "ngsi9", "contextEntityTypes", "*", "attributes"  }, "", badVerbGetPostOnly    },
  { "* InvalidRequest",                         "*",    InvalidRequest,                       0, { "*", "*", "*", "*", "*", "*"                      }, "", badRequest            },
  { "* *",                                      "",     InvalidRequest,                       0, {                                                   }, "", NULL                  }
};



/* ****************************************************************************
*
* nothingFound - 
*/
TEST(getContextEntityTypeAttributeContainer, nothingFound)
{
  ConnectionInfo ci("/ngsi9/contextEntityTypes/TYPE_123/attributes",  "GET", "1.1");
  std::string    expected = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>404</code>\n    <reasonPhrase>No context element registrations found</reasonPhrase>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";

  std::string    out;

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime()).WillByDefault(Return(1360232700));
  setTimer(timerMock);

  setupDatabase();

  ci.outFormat = XML;
  out          = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());

  delete timerMock;
}



/* ****************************************************************************
*
* somethingFound - 
*/
TEST(getContextEntityTypeAttributeContainer, somethingFound)
{
  ConnectionInfo ci1("/ngsi9/registerContext",                     "POST", "1.1");
  ConnectionInfo ci2("/ngsi9/contextEntityTypes/Room/attributes",  "GET",  "1.1");
  const char*    registerXmlFile = "ngsi9.registerContextRequest.ok.valid.xml";
  std::string    expectedStart   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <registrationId>";
  std::string    expected2       = "<discoverContextAvailabilityResponse>\n  <contextRegistrationResponseList>\n    <contextRegistrationResponse>\n      <contextRegistration>\n        <entityIdList>\n          <entityId type=\"Room\" isPattern=\"false\">\n            <id>ConferenceRoom</id>\n          </entityId>\n          <entityId type=\"Room\" isPattern=\"false\">\n            <id>OfficeRoom</id>\n          </entityId>\n        </entityIdList>\n        <contextRegistrationAttributeList>\n          <contextRegistrationAttribute>\n            <name>temperature</name>\n            <type>degree</type>\n            <isDomain>false</isDomain>\n          </contextRegistrationAttribute>\n        </contextRegistrationAttributeList>\n        <providingApplication>http://localhost:1028/application</providingApplication>\n      </contextRegistration>\n    </contextRegistrationResponse>\n  </contextRegistrationResponseList>\n</discoverContextAvailabilityResponse>\n";

  std::string    out;

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime()).WillByDefault(Return(1360232700));
  setTimer(timerMock);

  // Avoid forwarding of messages
  extern int fwdPort;
  int saved = fwdPort;

  setupDatabase();


  //
  // 1. Register entities so we have something to find:
  //    - entityId type="Room" isPattern="false", ConferenceRoom
  //    - entityId type="Room" isPattern="false", OfficeRoom
  //
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), registerXmlFile)) << "Error getting test data from '" << registerXmlFile << "'";

  ci1.outFormat    = XML;
  ci1.inFormat     = XML;
  ci1.payload      = testBuf;
  ci1.payloadSize  = strlen(testBuf);
  out              = restService(&ci1, rs);
  LM_M(("OUT: '%s'", out.c_str()));

  char* outStart  = (char*) out.c_str();
  outStart[expectedStart.length()] = 0;
  EXPECT_EQ(expectedStart, outStart);

  //
  // Now discover
  //
  ci2.outFormat = XML;
  out           = restService(&ci2, rs);

  EXPECT_EQ(expected2, out);

  // Putting old value back
  fwdPort = saved;

  delete timerMock;
}
