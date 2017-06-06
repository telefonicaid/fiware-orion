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
#include <string>

#include "logMsg/logMsg.h"

#include "serviceRoutines/postSubscribeContext.h"
#include "serviceRoutines/putSubscriptionConvOp.h"
#include "serviceRoutines/deleteSubscriptionConvOp.h"
#include "serviceRoutines/badVerbPutDeleteOnly.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* rs -
*/
#define UCSR   "updateContextSubscriptionRequest"
#define N10SCO Ngsi10SubscriptionsConvOp
#define SC     SubscribeContext
#define IR     InvalidRequest
static RestService rs[] =
{
  { "POST",   SC,     2, { "ngsi10", "subscribeContext"          }, "",   postSubscribeContext     },
  { "PUT",    N10SCO, 3, { "ngsi10", "contextSubscriptions", "*" }, UCSR, putSubscriptionConvOp    },
  { "DELETE", N10SCO, 3, { "ngsi10", "contextSubscriptions", "*" }, "",   deleteSubscriptionConvOp },
  { "*",      N10SCO, 3, { "ngsi10", "contextSubscriptions", "*" }, "",   badVerbPutDeleteOnly     },
  { "*",      IR,     0, { "*", "*", "*", "*", "*", "*"          }, "",   badRequest               },
  { "",       IR,     0, {                                       }, "",   NULL                     }
};




/* ****************************************************************************
*
* put -
*/
TEST(putSubscriptionConvOp, put)
{
  ConnectionInfo ci1("/ngsi10/contextSubscriptions/012345678901234567890123",  "XVERB",  "1.1");
  std::string    out;

  utInit();

  ci1.outMimeType    = JSON;
  ci1.inMimeType     = JSON;
  ci1.payload        = NULL;
  ci1.payloadSize    = 0;
  out                = restService(&ci1, rs);

  EXPECT_EQ("", out);
  EXPECT_EQ("Allow",       ci1.httpHeader[0]);
  EXPECT_EQ("PUT, DELETE", ci1.httpHeaderValue[0]);

  utExit();
}
