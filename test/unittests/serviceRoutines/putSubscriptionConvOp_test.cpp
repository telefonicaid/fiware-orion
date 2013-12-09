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

#include "serviceRoutines/postSubscribeContext.h"
#include "serviceRoutines/putSubscriptionConvOp.h"
#include "serviceRoutines/deleteSubscriptionConvOp.h"
#include "serviceRoutines/badVerbPutDeleteOnly.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"


#include "testDataFromFile.h"
#include "commonMocks.h"
#include "testInit.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;


/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST",   SubscribeContext,           2, { "ngsi10", "subscribeContext"           }, "",                                 postSubscribeContext     },
  { "PUT",    Ngsi10SubscriptionsConvOp,  3, { "ngsi10", "contextSubscriptions", "*"  }, "updateContextSubscriptionRequest", putSubscriptionConvOp    },
  { "DELETE", Ngsi10SubscriptionsConvOp,  3, { "ngsi10", "contextSubscriptions", "*"  }, "",                                 deleteSubscriptionConvOp },
  { "*",      Ngsi10SubscriptionsConvOp,  3, { "ngsi10", "contextSubscriptions", "*"  }, "",                                 badVerbPutDeleteOnly     },
  { "*",      InvalidRequest,             0, { "*", "*", "*", "*", "*", "*"           }, "",                                 badRequest               },
  { "",       InvalidRequest,             0, {                                        }, "",                                 NULL                     }
};
     



/* ****************************************************************************
*
* put - 
*/
TEST(putSubscriptionConvOp, put)
{
  ConnectionInfo ci1("/ngsi10/contextSubscriptions/012345678901234567890123",  "DELETE", "1.1");
  ConnectionInfo ci2("/ngsi10/contextSubscriptions/111222333444555666777888",  "PUT",    "1.1");
  ConnectionInfo ci3("/ngsi10/contextSubscriptions/111222333444555666777881",  "PUT",    "1.1");
  ConnectionInfo ci4("/ngsi10/contextSubscriptions/012345678901234567890123",  "XVERB",  "1.1");
  std::string    expected1      = "<unsubscribeContextResponse>\n  <subscriptionId>012345678901234567890123</subscriptionId>\n  <statusCode>\n    <code>404</code>\n    <reasonPhrase>Subscription Not Found</reasonPhrase>\n  </statusCode>\n</unsubscribeContextResponse>\n";
  std::string    expected2      = "<updateContextSubscriptionResponse>\n  <subscribeError>\n    <subscriptionId>111222333444555666777888</subscriptionId>\n    <errorCode>\n      <code>404</code>\n      <reasonPhrase>Subscription Not Found</reasonPhrase>\n    </errorCode>\n  </subscribeError>\n</updateContextSubscriptionResponse>\n";
  std::string    expected3      = "<updateContextSubscriptionResponse>\n  <subscribeError>\n    <subscriptionId>0</subscriptionId>\n    <errorCode>\n      <code>400</code>\n      <reasonPhrase>unmatching subscriptionId URI/payload</reasonPhrase>\n      <details>111222333444555666777881</details>\n    </errorCode>\n  </subscribeError>\n</updateContextSubscriptionResponse>\n";
  std::string    expected4      = "";
  const char*    fileName       = "ngsi10.updateContextSubscriptionRequest.subscriptionNotFound.valid.xml";
  std::string    out;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  setupDatabase();

  NotifierMock* notifierMock = new NotifierMock();
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  ci1.outFormat    = XML;
  ci1.inFormat     = XML;
  ci1.payload      = NULL;
  ci1.payloadSize  = 0;
  out              = restService(&ci1, rs);
  EXPECT_EQ(expected1, out);

  ci2.outFormat    = XML;
  ci2.inFormat     = XML;
  ci2.payload      = testBuf;
  ci2.payloadSize  = strlen(testBuf);
  out              = restService(&ci2, rs);
  EXPECT_EQ(expected2, out);

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";
  ci3.outFormat    = XML;
  ci3.inFormat     = XML;
  ci3.payload      = testBuf;
  ci3.payloadSize  = strlen(testBuf);
  out              = restService(&ci3, rs);
  EXPECT_EQ(expected3, out);

  ci4.outFormat    = XML;
  ci4.inFormat     = XML;
  ci4.payload      = NULL;
  ci4.payloadSize  = 0;
  out              = restService(&ci4, rs);
  EXPECT_EQ(expected4, out);
  EXPECT_EQ("Allow",       ci4.httpHeader[0]);
  EXPECT_EQ("PUT, DELETE", ci4.httpHeaderValue[0]);

  delete timerMock;
  delete notifierMock;
}
