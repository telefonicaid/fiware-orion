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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi9/DiscoverContextAvailabilityResponse.h"

#include "unittest.h"




/* ****************************************************************************
*
* EMPTY_JSON -
*/
#define EMPTY_JSON "{\n}\n"



/* ****************************************************************************
*
* render -
*
*/
TEST(DiscoverContextAvailabilityResponse, render)
{
  DiscoverContextAvailabilityResponse  dcar1;
  std::string                          out;
  StatusCode                           ec(SccBadRequest, "Detail");
  DiscoverContextAvailabilityResponse  dcar2(ec);

  utInit();

  out = dcar1.toJsonV1();
  EXPECT_EQ(SccReceiverInternalError, dcar1.errorCode.code);

  out = dcar2.toJsonV1();
  EXPECT_EQ(SccBadRequest, dcar2.errorCode.code);

  utExit();
}



/* ****************************************************************************
*
* jsonRender -
*
* NOTE
*   - providingApplication is MANDATORY inside ContextRegistration
*/
TEST(DiscoverContextAvailabilityResponse, jsonRender)
{
  // FIXME P2: gaps in numbering, rename
  // FIXME P2: review files. After removing medatata, isDomain, etc. some of them could be equal
  const char*                           filename1  = "ngsi9.discoverContextAvailabilityResponse.jsonRender1.valid.json";
  const char*                           filename2  = "ngsi9.discoverContextAvailabilityResponse.jsonRender2.valid.json";
  const char*                           filename3  = "ngsi9.discoverContextAvailabilityResponse.jsonRender3.valid.json";
  const char*                           filename4  = "ngsi9.discoverContextAvailabilityResponse.jsonRender4.valid.json";
  const char*                           filename5  = "ngsi9.discoverContextAvailabilityResponse.jsonRender5.valid.json";
  const char*                           filename6  = "ngsi9.discoverContextAvailabilityResponse.jsonRender6.valid.json";
  const char*                           filename7  = "ngsi9.discoverContextAvailabilityResponse.jsonRender7.valid.json";    
  const char*                           filename10 = "ngsi9.discoverContextAvailabilityResponse.jsonRender10.valid.json";
  const char*                           filename14 = "ngsi9.discoverContextAvailabilityResponse.jsonRender14.valid.json";
  const char*                           filename16 = "ngsi9.discoverContextAvailabilityResponse.jsonRender16.valid.json";
  const char*                           filename18 = "ngsi9.discoverContextAvailabilityResponse.jsonRender18.valid.json";
  const char*                           filename19 = "ngsi9.discoverContextAvailabilityResponse.jsonRender19.valid.json";
  const char*                           filename20 = "ngsi9.discoverContextAvailabilityResponse.jsonRender20.valid.json";
  const char*                           emptyFilename = "ngsi9.discoverContextAvailabilityResponse.jsonRender.empty.valid.json";
  std::string                           rendered;
  DiscoverContextAvailabilityResponse*  dcarP      = new DiscoverContextAvailabilityResponse();
  ContextRegistrationResponse*          crrP;
  EntityId*                             eidP;
  ContextRegistrationAttribute*         attrP;
  Metadata*                             mdP;

  utInit();

  // 1. One contextRegistrationResponse - no errorCode inside ContextRegistrationResponse
  crrP = new ContextRegistrationResponse();
  eidP = new EntityId("E01", "EType", "false");

  crrP->contextRegistration.entityIdVector.push_back(eidP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest");
  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release();  // ... otherwise the 500 remains and "pollutes" next tests
  free(dcarP);


  // 2. One contextRegistrationResponse - errorCode inside ContextRegistrationResponse - only providingApplication in ContextRegistration
  dcarP = new DiscoverContextAvailabilityResponse();
  crrP  = new ContextRegistrationResponse();

  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest2");
  crrP->errorCode.fill(SccBadRequest, "errorCode inside ContextRegistrationResponse");
  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  // No release here - the data stays - to be used in the following test scenario



  // 3. Two contextRegistrationResponses - one with errorCode and one without errorCode
  //    We're reusing the ContextRegistrationResponse from test 2 (it has StatusCode set
  crrP  = new ContextRegistrationResponse();
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest3");

  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename3)) << "Error getting test data from '" << filename3 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release();  // ... otherwise the 500 remains and "pollutes" next tests


  // 4.  ContextRegistration: One entityId inside entityIdVector
  crrP  = new ContextRegistrationResponse();
  eidP  = new EntityId("E04", "EType", "false");

  crrP->contextRegistration.entityIdVector.push_back(eidP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest4");
  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename4)) << "Error getting test data from '" << filename4 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  // No release here - the data stays - to be used in the following test scenario


  // 5.  ContextRegistration: Two entityIds inside entityIdVector
  eidP  = new EntityId("E05", "EType", "false");

  crrP->contextRegistration.entityIdVector.push_back(eidP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest5");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename5)) << "Error getting test data from '" << filename5 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release();  // ... otherwise the 500 remains and "pollutes" next tests


  // 6.  ContextRegistration: one attribute in contextRegistrationAttributeVector
  crrP  = new ContextRegistrationResponse();
  attrP = new ContextRegistrationAttribute("Attr1", "AType");

  crrP->contextRegistration.contextRegistrationAttributeVector.push_back(attrP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest6");
  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename6)) << "Error getting test data from '" << filename6 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  // No release here - the data stays - to be used in the following test scenario



  // 7.  ContextRegistration: two attributes in contextRegistrationAttributeVector
  attrP = new ContextRegistrationAttribute("Attr2", "AType");

  crrP->contextRegistration.contextRegistrationAttributeVector.push_back(attrP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest7");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename7)) << "Error getting test data from '" << filename7 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release();  // ... otherwise the 500 remains and "pollutes" next tests


  // 10. !entityIdVector !contextRegistrationAttributeVector +providingApplication
  crrP = new ContextRegistrationResponse();

  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest10");
  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename10)) << "Error getting test data from '" << filename10 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release(); // ... otherwise the 500 remains and "pollutes" next tests



  // 12. !entityIdVector +contextRegistrationAttributeVector +providingApplication
  crrP  = new ContextRegistrationResponse();
  attrP = new ContextRegistrationAttribute("Attr12", "AType");

  crrP->contextRegistration.contextRegistrationAttributeVector.push_back(attrP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest12");
  dcarP->responseVector.push_back(crrP);

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release(); // ... otherwise the 500 remains and "pollutes" next tests



  // 14. +entityIdVector !contextRegistrationAttributeVector +providingApplication
  crrP  = new ContextRegistrationResponse();
  eidP  = new EntityId("E14", "EType", "false");

  crrP->contextRegistration.entityIdVector.push_back(eidP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest14");
  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename14)) << "Error getting test data from '" << filename14 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release(); // ... otherwise the 500 remains and "pollutes" next tests



  // 16. +entityIdVector +contextRegistrationAttributeVector +providingApplication
  crrP  = new ContextRegistrationResponse();
  eidP  = new EntityId("E16", "EType", "false");
  attrP = new ContextRegistrationAttribute("Attr16", "AType");

  crrP->contextRegistration.entityIdVector.push_back(eidP);
  crrP->contextRegistration.contextRegistrationAttributeVector.push_back(attrP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest16");
  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename16)) << "Error getting test data from '" << filename16 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release();  // ... otherwise the 500 remains and "pollutes" next tests


  // 18. StatusCode
  dcarP->errorCode.fill(SccBadRequest, "DiscoverContextAvailabilityResponse Unit Test 18");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename18)) << "Error getting test data from '" << filename18 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  rendered = dcarP->toJsonV1();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release();  // ... otherwise the 500 remains and "pollutes" next tests


  // 19. StatusCode
  dcarP->errorCode.fill(SccBadRequest);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename19)) << "Error getting test data from '" << filename19 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  dcarP->release();  // ... otherwise the 500 remains and "pollutes" next tests


  // 20. Two ContextRegistrationResponses
  crrP = new ContextRegistrationResponse();
  eidP = new EntityId("E01", "EType", "false");

  crrP->contextRegistration.entityIdVector.push_back(eidP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest");
  dcarP->responseVector.push_back(crrP);

  crrP = new ContextRegistrationResponse();
  eidP = new EntityId("E02", "EType", "false");

  crrP->contextRegistration.entityIdVector.push_back(eidP);
  crrP->contextRegistration.providingApplication.set("http://tid.test.com/unitTest2");
  dcarP->responseVector.push_back(crrP);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename20)) << "Error getting test data from '" << filename20 << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  dcarP->release();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), emptyFilename)) << "Error getting test data from '" << emptyFilename << "'";
  rendered = dcarP->toJsonV1();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  free(dcarP);

  utExit();
}
