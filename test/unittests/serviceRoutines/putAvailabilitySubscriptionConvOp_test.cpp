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

#include "serviceRoutines/badVerbPutDeleteOnly.h"
#include "serviceRoutines/badVerbPostOnly.h"
#include "serviceRoutines/badRequest.h"
#include "serviceRoutines/postSubscribeContextAvailability.h"
#include "serviceRoutines/putAvailabilitySubscriptionConvOp.h"
#include "serviceRoutines/deleteAvailabilitySubscriptionConvOp.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST",   SubscribeContextAvailability, 2, { "ngsi9", "contextAvailabilitySubscriptions"      }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability         },
  { "*",      SubscribeContextAvailability, 2, { "ngsi9", "contextAvailabilitySubscriptions"      }, "",                                             badVerbPostOnly                          },

  { "PUT",    Ngsi9SubscriptionsConvOp,     3, { "ngsi9", "contextAvailabilitySubscriptions", "*" }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp        },
  { "DELETE", Ngsi9SubscriptionsConvOp,     3, { "ngsi9", "contextAvailabilitySubscriptions", "*" }, "",                                             deleteAvailabilitySubscriptionConvOp     },
  { "*",      Ngsi9SubscriptionsConvOp,     3, { "ngsi9", "contextAvailabilitySubscriptions", "*" }, "",                                             badVerbPutDeleteOnly                     },

  { "*",     InvalidRequest,                0, { "*", "*", "*", "*", "*", "*"                     }, "",                                 badRequest                                           },
  { "",      InvalidRequest,                0, {                                                  }, "",                                 NULL                                                 }
};
     



/* ****************************************************************************
*
* put - 
*
* FIXME P5 #1862: _json countepart?
*/
TEST(putAvailabilitySubscriptionConvOp, DISABLED_put)
{
  ConnectionInfo ci1("/ngsi9/contextAvailabilitySubscriptions",  "POST", "1.1");
  ConnectionInfo ci2("/ngsi9/contextAvailabilitySubscriptions",  "GET",  "1.1");
  ConnectionInfo ci3("/ngsi9/contextAvailabilitySubscriptions/111222333444555666777888",  "PUT",     "1.1");
  ConnectionInfo ci4("/ngsi9/contextAvailabilitySubscriptions/111222333444555666777888",  "DELETE",  "1.1");
  ConnectionInfo ci5("/ngsi9/contextAvailabilitySubscriptions/111222333444555666777881",  "PUT",     "1.1");
  ConnectionInfo ci6("/ngsi9/contextAvailabilitySubscriptions/012345678901234567890123",  "XVERB",   "1.1");
  const char*    infile1      = "ngsi9.subscribeContextAvailabilityRequest.ok.valid.xml";
  const char*    infile2      = "ngsi9.updateContextAvailabilitySubscriptionRequest.withSubId.valid.xml";
  const char*    outfile1     = "ngsi9.subscribeContextAvailabilityResponse.ok.middle.xml";
  const char*    outfile3     = "ngsi9.updateContextAvailabilitySubscriptionResponse.putAvailabilitySubscriptionConvOp.notFound.valid.xml";
  const char*    outfile4     = "ngsi9.unsubscribeContextAvailabilityResponse.putAvailabilitySubscriptionConvOp.notFound2.valid.xml";
  const char*    outfile5     = "ngsi9.updateContextAvailabilitySubscriptionResponse.putAvailabilitySubscriptionConvOp.unmatchingSubscriptionId.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile1)) << "Error getting test data from '" << infile1 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";

  ci1.payload      = testBuf;
  ci1.payloadSize  = strlen(testBuf);
  out              = restService(&ci1, rs);

  char* outStart   = (char*) out.c_str();
  // Remove last char in expectedBuf
  expectedBuf[strlen(expectedBuf) - 1] = 0;

  // Shorten'out' to be of same length as expectedBuf
  outStart[strlen(expectedBuf)]    = 0;
  EXPECT_STREQ(expectedBuf, out.c_str());


  out = restService(&ci2, rs);
  EXPECT_EQ("", out);
  EXPECT_EQ("Allow", ci2.httpHeader[0]);
  EXPECT_EQ("POST",  ci2.httpHeaderValue[0]);


  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile2)) << "Error getting test data from '" << infile2 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";

  ci3.payload      = testBuf;
  ci3.payloadSize  = strlen(testBuf);
  out              = restService(&ci3, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile4)) << "Error getting test data from '" << outfile4 << "'";
  out = restService(&ci4, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile2)) << "Error getting test data from '" << infile2 << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile5)) << "Error getting test data from '" << outfile5 << "'";
  ci5.payload      = testBuf;
  ci5.payloadSize  = strlen(testBuf);
  out              = restService(&ci5, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());


  out = restService(&ci6, rs);
  EXPECT_EQ("", out);
  EXPECT_EQ("Allow", ci6.httpHeader[0]);
  EXPECT_EQ("PUT, DELETE", ci6.httpHeaderValue[0]);

  utExit();
}
