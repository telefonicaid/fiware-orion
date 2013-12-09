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

#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/postDiscoverContextAvailability.h"
#include "serviceRoutines/badVerbPostOnly.h"

#include "rest/restReply.h"
#include "rest/ConnectionInfo.h"
#include "rest/RestService.h"

#include "unittest.h"



RestService rs[] =
{
  // NGSI-9 Requests
  { "POST",   RegisterContext,                       2, { "ngsi9",  "registerContext"                          }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                       2, { "ngsi9",  "registerContext"                          }, "registerContextRequest",                       badVerbPostOnly                           },

  // End marker for the array
  { "",       InvalidRequest,                        0, {                                                      }, "",                                             NULL                                      }
};


RestService rs2[] =
{
  // NGSI-9 Requests
  { "POST",   RegisterContext,                       2, { "ngsi9",  "registerContext"                          }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                       2, { "ngsi9",  "registerContext"                          }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,           2, { "ngsi9",  "discoverContextAvailability"              }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,           2, { "ngsi9",  "discoverContextAvailability"              }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },

  // End marker for the array
  { "",       InvalidRequest,                        0, {                                                      }, "",                                             NULL                                      }
};



/* ****************************************************************************
*
* payloadParse - 
*/
TEST(RestService, payloadParse)
{
  ConnectionInfo  ci("/ngsi9/registerContext", "POST", "1.1");
  ParseData       parseData;
  const char*     fileName1  = "ngsi9.registerContext.ok.valid.json";
  std::string     expected1  = "OK";
  std::string     expected2  = "Bad inFormat";
  std::string     out;

  utInit();

  //
  // 1. JSON
  //
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName1)) << "Error getting test data from '" << fileName1 << "'";

  ci.inFormat     = JSON;
  ci.outFormat    = JSON;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);

  out = payloadParse(&ci, &parseData, &rs[0], NULL, NULL);
  EXPECT_EQ(expected1, out);


  //
  // 2. NOFORMAT
  //
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName1)) << "Error getting test data from '" << fileName1 << "'";

  ci.inFormat     = NOFORMAT;
  ci.outFormat    = JSON;
  ci.payload      = (char*) "123";
  ci.payloadSize  = strlen(ci.payload);

  out = payloadParse(&ci, &parseData, &rs[0], NULL, NULL);  // Call restService?
  EXPECT_EQ(expected2, out);

  utExit();
}



/* ****************************************************************************
*
* noSuchService - 
*/
TEST(RestService, noSuchServiceAndNotFound)
{
  ConnectionInfo ci("/ngsi9/discoverContextAvailability",  "POST", "1.1");
  std::string    expected    = "{\n  \"registrationId\" : \"0\",\n  \"errorCode\" : {\n    \"code\" : \"400\",\n    \"reasonPhrase\" : \"bad request\",\n    \"details\" : \"Service not recognized\"\n  }\n}\n";
  std::string    expected2   = "{\n  \"errorCode\" : {\n    \"code\" : \"404\",\n    \"reasonPhrase\" : \"No context element registrations found\"\n  }\n}\n";
  const char*    fileName    = "ngsi9.discoverContextAvailabilityRequest.ok.valid.json";
  std::string    out;

  utInit();

  // No such service
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  ci.outFormat    = JSON;
  ci.inFormat     = JSON;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs);
  EXPECT_EQ(expected, out);

  // Not found
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  ci.outFormat    = JSON;
  ci.inFormat     = JSON;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs2);
  EXPECT_EQ(expected2, out);

  utExit();
}
