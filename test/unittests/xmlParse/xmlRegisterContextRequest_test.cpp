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

#include "xmlParse/xmlRequest.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"

#include "unittest.h"



/* ****************************************************************************
*
* association - 
*/
TEST(xmlRegisterContextRequest, association)
{
  ConnectionInfo  ci("/ngsi9/registerContext", "POST", "1.1");
  const char*     fileName = "ngsi9.registerContextRequest.entityAssociation.postponed.xml"; 
  ParseData       parseData;
  std::string     expected = "OK";
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expected, out);

  utExit();
}



/* ****************************************************************************
*
* invalidSourceEntityId - 
*/
TEST(xmlRegisterContextRequest, invalidSourceEntityId)
{
  ConnectionInfo  ci("/ngsi9/registerContext", "POST", "1.1");
  const char*     fileName = "ngsi9.registerContextRequest.invalidSourceEntityId.invalid.xml"; 
  ParseData       parseData;
  std::string     expected = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expected, out);

  utExit();
}



/* ****************************************************************************
*
* invalidTargetEntityId - 
*/
TEST(xmlRegisterContextRequest, invalidTargetEntityId)
{
  ConnectionInfo  ci("/ngsi9/registerContext", "POST", "1.1");
  const char*     fileName = "ngsi9.registerContextRequest.invalidTargetEntityId.invalid.xml"; 
  ParseData       parseData;
  std::string     expected = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unsupported attribute for EntityId</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expected, out);

  utExit();
}



/* ****************************************************************************
*
* twoEntityIdLists - 
*/
TEST(xmlRegisterContextRequest, twoEntityIdLists)
{
  ConnectionInfo  ci("/ngsi9/registerContext", "POST", "1.1");
  const char*     fileName = "ngsi9.registerContextRequest.twoEntityIdLists.invalid.xml"; 
  ParseData       parseData;
  std::string     expected = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Got an entityIdList when one was present already</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, RegisterContext, "registerContextRequest", NULL);
  EXPECT_EQ(expected, out);

  utExit();
}
