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

#include "unittests/unittest.h"

#include "logMsg/logMsg.h"

#include "serviceRoutines/getIndividualContextEntityAttributes.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"
#include "rest/rest.h"



/* ****************************************************************************
*
* service vectors -
*/
static RestService getV[] =
{
  { IndividualContextEntityAttributes, 4, { "ngsi10", "contextEntities", "*", "attributes" }, "", getIndividualContextEntityAttributes },
  { InvalidRequest,                    0, {                                                }, "", NULL                                 }
};

static RestService badVerbV[] =
{
  { InvalidRequest,   0, { "*", "*", "*", "*", "*", "*"                   }, "", badRequest                           },
  { InvalidRequest,   0, {                                                }, "", NULL                                 }
};



/* ****************************************************************************
*
* notFound -
*/
TEST(getIndividualContextEntityAttributes, notFound)
{
  ConnectionInfo ci("/ngsi10/contextEntities/entity11/attributes",  "GET", "1.1");
  const char*    outfile = "ngsi10.contextElementResponse.getIndividualContextEntityAttributes.notFound.valid.xml";
  std::string    out;

  utInit();

  ci.outMimeType = JSON;

  serviceVectorsSet(getV, NULL, NULL, NULL, NULL, NULL, badVerbV);
  out = orionServe(&ci);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile)) << "Error getting test data from '" << outfile << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
