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
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST",   RegisterContext,                       2, { "ngsi9",  "registerContext"                          }, "", postRegisterContext                       },
  { "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* ok - 
*/
TEST(postRegisterContext, ok)
{
  ConnectionInfo ci("/ngsi9/registerContext",  "POST", "1.1");
  ConnectionInfo ci2("/ngsi9/registerContext",  "POST", "1.1");
  std::string    expectedStart   = "<registerContextResponse>\n  <duration>PT1H</duration>\n  <registrationId>";
  std::string    expected2       = "<registerContextResponse>\n  <registrationId>012345678901234567890123</registrationId>\n  <errorCode>\n    <code>404</code>\n    <reasonPhrase>Registration Not Found</reasonPhrase>\n  </errorCode>\n</registerContextResponse>\n";
  const char*    fileName        = "ngsi9.registerContextRequest.ok.valid.xml";
  const char*    fileName2       = "ngsi9.registerContextRequest.update.valid.xml";
  std::string    out;

  // Avoid forwarding of messages
  extern int fwdPort;
  int saved = fwdPort;
  fwdPort = 0;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  ci.outFormat    = XML;
  ci.inFormat     = XML;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs);

  char* outStart  = (char*) out.c_str();
  outStart[expectedStart.length()] = 0;
  EXPECT_STREQ(expectedStart.c_str(), outStart);

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName2)) << "Error getting test data from '" << fileName2 << "'";
  ci2.outFormat    = XML;
  ci2.inFormat     = XML;
  ci2.payload      = testBuf;
  ci2.payloadSize  = strlen(testBuf);
  out              = restService(&ci2, rs);
  EXPECT_EQ(expected2, out);

  // Putting old value back
  fwdPort = saved;

  utExit();
}
