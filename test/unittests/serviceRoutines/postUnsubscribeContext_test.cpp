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

#include "serviceRoutines/postUnsubscribeContext.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "testDataFromFile.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST",   UnsubscribeContext,                    2, { "ngsi10", "unsubscribeContext"                       }, "", postUnsubscribeContext                    },
  { "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* badSubscriptionId - 
*/
TEST(postUnsubscribeContext, badSubscriptionId)
{
  ConnectionInfo ci("/ngsi10/unsubscribeContext",  "POST", "1.1");
  const char*    fileName    = "ngsi10.unsubscribeContextRequest.subscriptionId.invalid.xml";
  std::string    expected    = "<unsubscribeContextResponse>\n  <subscriptionId>000000000000000000000000</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>Invalid Subscription Id: bad length (24 chars expected)</reasonPhrase>\n    <details>12345D</details>\n  </statusCode>\n</unsubscribeContextResponse>\n";
  std::string    out;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.outFormat    = XML;
  ci.inFormat     = XML;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* notFound - 
*/
TEST(postUnsubscribeContext, notFound)
{
  ConnectionInfo ci("/ngsi10/unsubscribeContext",  "POST", "1.1");
  const char*    fileName    = "ngsi10.unsubscribeContextRequest.subscriptionId.valid.xml";
  std::string    expected    = "<unsubscribeContextResponse>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n  <statusCode>\n    <code>404</code>\n    <reasonPhrase>No context element found</reasonPhrase>\n    <details>subscriptionId: '012345678901234567890123'</details>\n  </statusCode>\n</unsubscribeContextResponse>\n";
  std::string    out;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  ci.outFormat    = XML;
  ci.inFormat     = XML;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());
}
