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

#include "serviceRoutines/badVerbPutDeleteOnly.h"
#include "serviceRoutines/badVerbPostOnly.h"
#include "serviceRoutines/badRequest.h"
#include "serviceRoutines/postSubscribeContextAvailability.h"
#include "serviceRoutines/putAvailabilitySubscriptionConvOp.h"
#include "serviceRoutines/deleteAvailabilitySubscriptionConvOp.h"
#include "rest/RestService.h"
#include "rest/rest.h"

#include "unittests/unittest.h"


/* ****************************************************************************
*
* service vectors -
*/
#define SCA    SubscribeContextAvailability
#define IR     InvalidRequest
#define N9SCO  Ngsi9SubscriptionsConvOp

static RestService postV[] =
{
  { SCA, 2, { "ngsi9", "contextAvailabilitySubscriptions" }, postSubscribeContextAvailability            },
  { IR,  0, {                                             }, NULL                                        }
};

static RestService putV[] =
{
  { N9SCO, 3, { "ngsi9", "contextAvailabilitySubscriptions", "*" }, putAvailabilitySubscriptionConvOp    },
  { IR,    0, {                                                  }, NULL                                 }
};

static RestService deleteV[] =
{
  { N9SCO, 3, { "ngsi9", "contextAvailabilitySubscriptions", "*" }, deleteAvailabilitySubscriptionConvOp },
  { IR,    0, {                                                  }, NULL                                 }
};

static RestService badVerbV[] =
{
  { SCA,   2, { "ngsi9", "contextAvailabilitySubscriptions"      }, badVerbPostOnly                      },
  { N9SCO, 3, { "ngsi9", "contextAvailabilitySubscriptions", "*" }, badVerbPutDeleteOnly                 },
  { IR,    0, { "*", "*", "*", "*", "*", "*"                     }, badRequest                           },
  { IR,    0, {                                                  }, NULL                                 }
};



/* ****************************************************************************
*
* put -
*/
TEST(putAvailabilitySubscriptionConvOp, put)
{
  ConnectionInfo ci1("/ngsi9/contextAvailabilitySubscriptions",  "GET",  "1.1");
  ConnectionInfo ci2("/ngsi9/contextAvailabilitySubscriptions/012345678901234567890123",  "XVERB",   "1.1");
  std::string    out;
  RestService    restService1 =
    {
      ExitRequest,
      2, { "/ngsi9", "contextAvailabilitySubscriptions" },
      NULL
    };
  RestService    restService2 =
    {
      ExitRequest,
      3, { "/ngsi9", "contextAvailabilitySubscriptions", "012345678901234567890123" },
      NULL
    };

  utInit();

  serviceVectorsSet(NULL, putV, postV, NULL, deleteV, NULL, badVerbV);
  ci1.apiVersion   = V1;
  ci1.restServiceP = &restService1;
  out = orion::requestServe(&ci1);

  EXPECT_EQ("", out);
  EXPECT_EQ("Allow", ci1.httpHeader[0]);
  EXPECT_EQ("POST",  ci1.httpHeaderValue[0]);

  ci2.apiVersion   = V1;
  ci2.restServiceP = &restService2;
  out = orion::requestServe(&ci2);
  EXPECT_EQ("", out);
  EXPECT_EQ("Allow", ci2.httpHeader[0]);
  EXPECT_EQ("PUT, DELETE", ci2.httpHeaderValue[0]);

  utExit();
}
