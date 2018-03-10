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
#include "rest/rest.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* badVerbV -
*/
static RestService badVerbV[] =
{
  { Ngsi10SubscriptionsConvOp,  3, { "ngsi10", "contextSubscriptions", "*" }, "", badVerbPutDeleteOnly },
  { InvalidRequest,             0, { "*", "*", "*", "*", "*", "*"          }, "", badRequest           },
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

  serviceVectorsSet(NULL, NULL, NULL, NULL, NULL, NULL, badVerbV);
  out = orion::requestServe(&ci1);

  EXPECT_EQ("Allow",       ci1.httpHeader[0]);
  EXPECT_EQ("PUT, DELETE", ci1.httpHeaderValue[0]);
  EXPECT_EQ("", out);

  utExit();
}
