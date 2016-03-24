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

#include "serviceRoutines/postContextEntityTypes.h"
#include "serviceRoutines/badVerbGetPostOnly.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST", ContextEntityTypes, 3, { "ngsi9", "contextEntityTypes", "*" }, "", postContextEntityTypes },
  { "*",    ContextEntityTypes, 3, { "ngsi9", "contextEntityTypes", "*" }, "", badVerbGetPostOnly     },
  { "*",    InvalidRequest,     0, { "*", "*", "*", "*", "*", "*"       }, "", badRequest             },
  { "",     InvalidRequest,     0, {                                    }, "", NULL                   }
};



/* ****************************************************************************
*
* ok - 
*
* FIXME P5 #1862: _json counterpart?
*/
TEST(postContextEntityTypes, DISABLED_ok)
{
  utInit();

  // Avoid forwarding of messages
  extern int fwdPort;
  int saved = fwdPort;
  fwdPort = 0;
  
  ConnectionInfo ci("/ngsi9/contextEntityTypes/TYPE_123",  "POST", "1.1");
  const char*    infile      = "ngsi9.registerProviderRequest.noRegistrationId.postponed.xml";
  const char*    outfile     = "ngsi9.registerContextResponse.noRegistrationId.middle.xml";
  std::string    out;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.outFormat    = JSON;
  ci.inFormat     = JSON;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs);

  char* outStart  = (char*) out.c_str();

  // Remove last char in expectedBuf
  expectedBuf[strlen(expectedBuf) - 1] = 0;

  // Shorten 'out' to be of same length as expectedBuf
  outStart[strlen(expectedBuf)]    = 0;
  EXPECT_STREQ(expectedBuf, out.c_str());

  fwdPort = saved;
  utExit();
}
