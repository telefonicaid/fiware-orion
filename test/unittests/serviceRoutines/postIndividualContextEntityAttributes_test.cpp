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

#include "serviceRoutines/postIndividualContextEntityAttributes.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* rs -
*/
#define ICEA IndividualContextEntityAttributes
#define IR   InvalidRequest
static RestService rs[] =
{
  { "POST", ICEA, 4, { "ngsi10", "contextEntities", "*", "attributes" }, "", postIndividualContextEntityAttributes },
  { "*",    IR,   0, { "*", "*", "*", "*", "*", "*"                   }, "", badRequest                            },
  { "",     IR,   0, {                                                }, "", NULL                                  }
};



/* ****************************************************************************
*
* createEntity -
*/
TEST(postIndividualContextEntityAttributes, createEntity)
{
  ConnectionInfo ci("/ngsi10/contextEntities/entity11/attributes",  "POST", "1.1");
  const char*    infile  = "ngsi10.appendContextElementRequest.ok.valid.xml";
  const char*    outfile = "ngsi10.appendContextElementResponse.ok.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf,
                                   sizeof(testBuf),
                                   infile)) << "Error getting test data from '" << infile << "'";

  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile)) << "Error getting test data from '" << outfile << "'";

  ci.outMimeType    = JSON;
  ci.inMimeType     = JSON;
  ci.payload        = testBuf;
  ci.payloadSize    = strlen(testBuf);
  out               = restService(&ci, rs);

  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
