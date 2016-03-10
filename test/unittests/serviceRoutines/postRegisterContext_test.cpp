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
*
* FIXME P5 #1862: _json counterpart?
*/
TEST(postRegisterContext, DISABLED_ok)
{
  ConnectionInfo ci("/ngsi9/registerContext",  "POST", "1.1");
  ConnectionInfo ci2("/ngsi9/registerContext",  "POST", "1.1");
  const char*    infile1   = "ngsi9.registerContextRequest.ok.valid.xml";
  const char*    infile2   = "ngsi9.registerContextRequest.update.valid.xml";
  const char*    outfile1  = "ngsi9.registerContextResponse.postRegisterContext1.middle.xml";
  const char*    outfile2  = "ngsi9.registerContextResponse.postRegisterContext2.valid.xml";
  std::string    out;

  // Avoid forwarding of messages
  extern int     fwdPort;
  int            saved = fwdPort;
  fwdPort = 0;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile1)) << "Error getting test data from '" << infile1 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";

  ci.outFormat    = JSON;
  ci.inFormat     = JSON;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs);

  char* outStart  = (char*) out.c_str();

  // Remove last char in expectedBuf
  expectedBuf[strlen(expectedBuf) - 1] = 0;

  // Shorten'out' to be of same length as expectedBuf
  outStart[strlen(expectedBuf)]    = 0;
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile2)) << "Error getting test data from '" << infile2 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  ci2.outFormat    = JSON;
  ci2.inFormat     = JSON;
  ci2.payload      = testBuf;
  ci2.payloadSize  = strlen(testBuf);
  out              = restService(&ci2, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  // Putting old value back
  fwdPort = saved;

  utExit();
}
