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

#include "serviceRoutines/postSubscribeContext.h"
#include "serviceRoutines/putSubscriptionConvOp.h"
#include "serviceRoutines/deleteSubscriptionConvOp.h"
#include "serviceRoutines/badVerbPutDeleteOnly.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittest.h"



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
*
* FIXME P5 #1862: _json counterpart?
*/
TEST(putSubscriptionConvOp, DISABLED_put)
{
  ConnectionInfo ci1("/ngsi10/contextSubscriptions/012345678901234567890123",  "DELETE", "1.1");
  ConnectionInfo ci2("/ngsi10/contextSubscriptions/111222333444555666777888",  "PUT",    "1.1");
  ConnectionInfo ci3("/ngsi10/contextSubscriptions/111222333444555666777881",  "PUT",    "1.1");
  ConnectionInfo ci4("/ngsi10/contextSubscriptions/012345678901234567890123",  "XVERB",  "1.1");
  const char*    infile       = "ngsi10.updateContextSubscriptionRequest.subscriptionNotFound.valid.xml";
  const char*    outfile1     = "ngsi10.unsubscribeContextResponse.putSubscriptionConvOp.notFound.valid.xml";
  const char*    outfile2     = "ngsi10.updateContextSubscriptionResponse.putSubscriptionConvOp.notFound.valid.xml";
  const char*    outfile3     = "ngsi10.updateContextSubscriptionResponse.putSubscriptionConvOp.unmatchingSubscriptionId.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  ci1.outMimeType    = JSON;
  ci1.inMimeType     = JSON;
  ci1.payload        = NULL;
  ci1.payloadSize    = 0;
  out                = restService(&ci1, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());
  

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  ci2.outMimeType    = JSON;
  ci2.inMimeType     = JSON;
  ci2.payload        = testBuf;
  ci2.payloadSize    = strlen(testBuf);
  out                = restService(&ci2, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  ci3.outMimeType    = JSON;
  ci3.inMimeType     = JSON;
  ci3.payload        = testBuf;
  ci3.payloadSize    = strlen(testBuf);
  out                = restService(&ci3, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());


  ci4.outMimeType    = JSON;
  ci4.inMimeType     = JSON;
  ci4.payload        = NULL;
  ci4.payloadSize    = 0;
  out                = restService(&ci4, rs);
  EXPECT_EQ("", out);
  EXPECT_EQ("Allow",       ci4.httpHeader[0]);
  EXPECT_EQ("PUT, DELETE", ci4.httpHeaderValue[0]);

  utExit();
}
