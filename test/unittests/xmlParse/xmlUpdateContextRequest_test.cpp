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

#include "xmlParse/xmlRequest.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "rest/ConnectionInfo.h"

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
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, UpdateContext, "updateContextRequest", NULL);
  EXPECT_EQ("OK", out);

  utExit();
}



/* ****************************************************************************
*
* invalidEntityIdAttribute - 
*/
TEST(xmlUpdateContextRequest, invalidEntityIdAttribute)
{
  ConnectionInfo  ci("/ngsi10/updateContext", "POST", "1.1");
  const char*     infile  = "ngsi10.entityIdAttribute.invalid.xml";
  const char*     outfile = "ngsi10.updateContextResponse.badRequest.valid.xml";
  ParseData       parseData;
  std::string     out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.inFormat  = XML;
  ci.outFormat = XML;
  out  = xmlTreat(testBuf, &ci, &parseData, UpdateContext, "updateContextRequest", NULL);
  EXPECT_STREQ(expectedBuf, out.c_str());
  EXPECT_EQ("unsupported attribute for EntityId", parseData.errorString);

  utExit();
}
