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

#include "serviceRoutines/postDiscoverContextAvailability.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST",   DiscoverContextAvailability,           2, { "ngsi9",  "discoverContextAvailability"              }, "", postDiscoverContextAvailability           },
  { "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* notFound - 
*
* FIXME P5 #1832: _json counterpart?
*/
TEST(postDiscoverContextAvailability, DISABLED_notFound)
{
  ConnectionInfo ci("/ngsi9/discoverContextAvailability",  "POST", "1.1");
  const char*    infile    = "ngsi9.discoverContextAvailabilityRequest.entityIdIdNotFound.valid.xml";
  const char*    outfile   = "ngsi9.discoverContextAvailabilityResponse.entityIdIdNotFound.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.outMimeType    = JSON;
  ci.inMimeType     = JSON;
  ci.payload        = testBuf;
  ci.payloadSize    = strlen(testBuf);
  out               = restService(&ci, rs);

  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
