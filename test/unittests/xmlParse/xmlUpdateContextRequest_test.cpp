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
* ok - 
*/
TEST(xmlUpdateContextRequest, ok)
{
  ConnectionInfo  ci("/ngsi10/updateContext", "POST", "1.1");
  const char*     fileName = "ngsi10.updateContextRequestWithMetadata.valid.xml";
  ParseData       parseData;
  std::string     expected = "OK";
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, UpdateContext, "updateContextRequest", NULL);
  EXPECT_EQ(expected, out);

  utExit();
}



/* ****************************************************************************
*
* invalidEntityIdAttribute - 
*/
TEST(xmlUpdateContextRequest, invalidEntityIdAttribute)
{
  ConnectionInfo  ci("/ngsi10/updateContext", "POST", "1.1");
  const char*     fileName = "ngsi10.entityIdAttribute.invalid.xml";
  ParseData       parseData;
  std::string     expected  = "<updateContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad request</reasonPhrase>\n    <details>unsupported attribute for EntityId</details>\n  </errorCode>\n</updateContextResponse>\n";
  std::string     expected2 = "unsupported attribute for EntityId";
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, UpdateContext, "updateContextRequest", NULL);
  EXPECT_EQ(expected, out);
  EXPECT_EQ(expected2, parseData.errorString);

  utExit();
}
