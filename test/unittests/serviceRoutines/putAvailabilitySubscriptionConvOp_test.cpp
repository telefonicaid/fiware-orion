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
*/
TEST(putAvailabilitySubscriptionConvOp, put)
{
  ConnectionInfo ci1("/ngsi9/contextAvailabilitySubscriptions",  "POST", "1.1");
  ConnectionInfo ci2("/ngsi9/contextAvailabilitySubscriptions",  "GET",  "1.1");
  ConnectionInfo ci3("/ngsi9/contextAvailabilitySubscriptions/111222333444555666777888",  "PUT",     "1.1");
  ConnectionInfo ci4("/ngsi9/contextAvailabilitySubscriptions/111222333444555666777888",  "DELETE",  "1.1");
  ConnectionInfo ci5("/ngsi9/contextAvailabilitySubscriptions/111222333444555666777881",  "PUT",     "1.1");
  ConnectionInfo ci6("/ngsi9/contextAvailabilitySubscriptions/012345678901234567890123",  "XVERB",   "1.1");
  std::string    expectedStart1 = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>";
  std::string    expected2      = "";
  std::string    expected3      = "<updateContextAvailabilitySubscriptionResponse>\n  <subscriptionId>111222333444555666777888</subscriptionId>\n  <errorCode>\n    <code>404</code>\n    <reasonPhrase>No context element found</reasonPhrase>\n  </errorCode>\n</updateContextAvailabilitySubscriptionResponse>\n";
  std::string    expected4      = "<unsubscribeContextAvailabilityResponse>\n  <subscriptionId>111222333444555666777888</subscriptionId>\n  <statusCode>\n    <code>404</code>\n    <reasonPhrase>No context element found</reasonPhrase>\n  </statusCode>\n</unsubscribeContextAvailabilityResponse>\n";
  std::string    expected5      = "<updateContextAvailabilitySubscriptionResponse>\n  <subscriptionId>000000000000000000000000</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>unmatching subscriptionId URI/payload</reasonPhrase>\n    <details>111222333444555666777881</details>\n  </errorCode>\n</updateContextAvailabilitySubscriptionResponse>\n";
  std::string    expected6      = "";
  const char*    fileName1      = "ngsi9.subscribeContextAvailabilityRequest.ok.valid.xml";
  const char*    fileName2      = "ngsi9.updateContextAvailabilitySubscriptionRequest.withSubId.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName1)) << "Error getting test data from '" << fileName1 << "'";
  ci1.payload      = testBuf;
  ci1.payloadSize  = strlen(testBuf);
  out              = restService(&ci1, rs);
  char* outStart   = (char*) out.c_str();
  outStart[strlen(expectedStart1.c_str())] = 0;
  EXPECT_EQ(expectedStart1, outStart);

  out = restService(&ci2, rs);
  EXPECT_EQ(expected2, out);
  EXPECT_EQ("Allow", ci2.httpHeader[0]);
  EXPECT_EQ("POST",  ci2.httpHeaderValue[0]);

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName2)) << "Error getting test data from '" << fileName2 << "'";
  ci3.payload      = testBuf;
  ci3.payloadSize  = strlen(testBuf);
  out              = restService(&ci3, rs);
  EXPECT_EQ(expected3, out);

  out = restService(&ci4, rs);
  EXPECT_EQ(expected4, out);

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName2)) << "Error getting test data from '" << fileName2 << "'";
  ci5.payload      = testBuf;
  ci5.payloadSize  = strlen(testBuf);
  out              = restService(&ci5, rs);
  EXPECT_EQ(expected5, out);

  out = restService(&ci6, rs);
  EXPECT_EQ(expected6, out);
  EXPECT_EQ("Allow", ci6.httpHeader[0]);
  EXPECT_EQ("PUT, DELETE", ci6.httpHeaderValue[0]);

  utExit();
}
