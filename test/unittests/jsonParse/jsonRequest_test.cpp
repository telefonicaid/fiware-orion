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
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "jsonParse/jsonRequest.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "rest/ConnectionInfo.h"
#include "rest/RestService.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* jsonTreat - 
*/
TEST(jsonRequest, jsonTreat)
{
  ConnectionInfo  ci("/ngsi9/registerContext", "POST", "1.1");
  ParseData       parseData;
  std::string     out;
  const char*     outfile1 = "orion.jsonRequest.jsonTreat.valid.json";
  RestService     restService = { InvalidRequest, 2, { "ngsi9", "registerContext" }, NULL };

  utInit();

  ci.outMimeType  = JSON;
  ci.apiVersion   = V1;
  ci.restServiceP = &restService;

  out  = jsonTreat("non-empty content", &ci, &parseData, InvalidRequest, NULL);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile1)) << "Error getting test data from '" << outfile1 << "'";

  EXPECT_STREQ(expectedBuf, out.c_str());

  out  = jsonTreat("", &ci, &parseData, InvalidRequest, NULL);
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}
