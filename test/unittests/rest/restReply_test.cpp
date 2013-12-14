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
#include "rest/restReply.h"
#include "rest/ConnectionInfo.h"

#include "unittest.h"



/* ****************************************************************************
*
* formatedAnswer - 
*/
TEST(restReply, formatedAnswer)
{
  std::string     expected  = "<statusCode>\n  <code>200</code>\n  <reasonPhrase>OK</reasonPhrase>\n</statusCode>";
  std::string     expected2 = "{\n  \"statusCode\":\n  {\n  \"code\": \"200\",\n  \"reasonPhrase\": \"OK\"\n  }\n}";
  std::string     expected3 = "statusCode: code=200, reasonPhrase=OK";
  std::string     out;

  utInit();

  out = formatedAnswer(XML, "statusCode", "code", "200", "reasonPhrase", "OK");
  EXPECT_EQ(expected, out);

  out = formatedAnswer(JSON, "statusCode", "code", "200", "reasonPhrase", "OK");
  EXPECT_EQ(expected2, out);

  out = formatedAnswer(NOFORMAT, "statusCode", "code", "200", "reasonPhrase", "OK");
  EXPECT_EQ(expected3, out);

  utExit();
}



/* ****************************************************************************
*
* MHD_create_response_from_data_error - 
*
* Too large response string 
*/
#define TEST_SIZE (4 * 1024 * 1024)
TEST(restReply, MHD_create_response_from_data_error)
{
  int             out;
  ConnectionInfo  ci("/ngsi/XXX", "GET", "1.1");
  char*           answer = (char*) malloc(TEST_SIZE);

  utInit();

  if (answer != NULL)
  {
    memset(answer, 'x', TEST_SIZE - 1);
    answer[TEST_SIZE - 1] = 0;

    out = restReply(&ci, answer);
    EXPECT_EQ(MHD_NO, out);

    free(answer);
  }

  utExit();
}



/* ****************************************************************************
*
* json - 
*/
TEST(restReply, json)
{
  ConnectionInfo  ci("/ngsi/XXX", "GET", "1.1");
  int             out;

  utInit();

  ci.outFormat = JSON;
  out = restReply(&ci, "123");
  EXPECT_EQ(MHD_YES, out);

  utExit();
}



/* ****************************************************************************
*
* restErrorReplyGet - 
*/
TEST(restReply, restErrorReplyGet)
{
  std::string rcr1 = "registerContext";
  std::string rcr2 = "/ngsi9/registerContext";
  std::string rcr3 = "/NGSI9/registerContext";
  std::string rcr4 = "registerContextRequest";
  std::string rcrExpected  = "<registerContextResponse>\n  <registrationId>0</registrationId>\n</registerContextResponse>\n";
  std::string rcrExpected2 = "<registerContextResponse>\n  <registrationId>0</registrationId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</registerContextResponse>\n";

  std::string dcar1 = "discoverContextAvailability";
  std::string dcar2 = "/ngsi9/discoverContextAvailability";
  std::string dcar3 = "/NGSI9/discoverContextAvailability";
  std::string dcar4 = "discoverContextAvailabilityRequest";
  std::string dcarExpected  = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>200</code>\n    <reasonPhrase>ok</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";
  std::string dcarExpected2 = "<discoverContextAvailabilityResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</discoverContextAvailabilityResponse>\n";

  std::string scar1 = "subscribeContextAvailability";
  std::string scar2 = "/ngsi9/subscribeContextAvailability";
  std::string scar3 = "/NGSI9/subscribeContextAvailability";
  std::string scar4 = "subscribeContextAvailabilityRequest";
  std::string scarExpected  = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>0</subscriptionId>\n  <errorCode>\n    <code>200</code>\n    <reasonPhrase>ok</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</subscribeContextAvailabilityResponse>\n";
  std::string scarExpected2 = "<subscribeContextAvailabilityResponse>\n  <subscriptionId>0</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</subscribeContextAvailabilityResponse>\n";

  std::string ucas1 = "updateContextAvailabilitySubscription";
  std::string ucas2 = "/ngsi9/updateContextAvailabilitySubscription";
  std::string ucas3 = "/NGSI9/updateContextAvailabilitySubscription";
  std::string ucas4 = "updateContextAvailabilitySubscriptionRequest";
  std::string ucasExpected  = "<updateContextAvailabilitySubscriptionResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <errorCode>\n    <code>200</code>\n    <reasonPhrase>ok</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</updateContextAvailabilitySubscriptionResponse>\n";
  std::string ucasExpected2 = "<updateContextAvailabilitySubscriptionResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</updateContextAvailabilitySubscriptionResponse>\n";
  
  std::string ucar1 = "unsubscribeContextAvailability";
  std::string ucar2 = "/ngsi9/unsubscribeContextAvailability";
  std::string ucar3 = "/NGSI9/unsubscribeContextAvailability";
  std::string ucar4 = "unsubscribeContextAvailabilityRequest";
  std::string ucarExpected  = "<unsubscribeContextAvailabilityResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <statusCode>\n    <code>200</code>\n    <reasonPhrase>ok</reasonPhrase>\n    <details>detail</details>\n  </statusCode>\n</unsubscribeContextAvailabilityResponse>\n";
  std::string ucarExpected2 = "<unsubscribeContextAvailabilityResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </statusCode>\n</unsubscribeContextAvailabilityResponse>\n";
  
  std::string ncar1 = "notifyContextAvailability";
  std::string ncar2 = "/ngsi9/notifyContextAvailability";
  std::string ncar3 = "/NGSI9/notifyContextAvailability";
  std::string ncar4 = "notifyContextAvailabilityRequest";
  std::string ncarExpected  = "<notifyContextAvailabilityResponse>\n  <responseCode>\n    <code>200</code>\n    <reasonPhrase>ok</reasonPhrase>\n    <details>detail</details>\n  </responseCode>\n</notifyContextAvailabilityResponse>\n";
  std::string ncarExpected2 = "<notifyContextAvailabilityResponse>\n  <responseCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </responseCode>\n</notifyContextAvailabilityResponse>\n";
  
  std::string qcr1 = "queryContext";
  std::string qcr2 = "/ngsi10/queryContext";
  std::string qcr3 = "/NGSI10/queryContext";
  std::string qcr4 = "queryContextRequest";
  std::string qcrExpected  = "<queryContextResponse>\n  <errorCode>\n    <code>404</code>\n    <reasonPhrase>Query without hits</reasonPhrase>\n  </errorCode>\n</queryContextResponse>\n";
  std::string qcrExpected2 = "<queryContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</queryContextResponse>\n";
  
  std::string scr1 = "subscribeContext";
  std::string scr2 = "/ngsi10/subscribeContext";
  std::string scr3 = "/NGSI10/subscribeContext";
  std::string scr4 = "subscribeContextRequest";
  std::string scrExpected  = "<subscribeContextResponse>\n  <subscribeError>\n    <errorCode>\n      <code>200</code>\n      <reasonPhrase>ok</reasonPhrase>\n      <details>detail</details>\n    </errorCode>\n  </subscribeError>\n</subscribeContextResponse>\n";
  std::string scrExpected2 = "<subscribeContextResponse>\n  <subscribeError>\n    <errorCode>\n      <code>400</code>\n      <reasonPhrase>Bad Request</reasonPhrase>\n      <details>detail</details>\n    </errorCode>\n  </subscribeError>\n</subscribeContextResponse>\n";
  
  std::string ucs1 = "updateContextSubscription";
  std::string ucs2 = "/ngsi10/updateContextSubscription";
  std::string ucs3 = "/NGSI10/updateContextSubscription";
  std::string ucs4 = "updateContextSubscriptionRequest";
  std::string ucsExpected  = "<updateContextSubscriptionResponse>\n  <subscribeError>\n    <subscriptionId>0</subscriptionId>\n    <errorCode>\n      <code>200</code>\n      <reasonPhrase>ok</reasonPhrase>\n      <details>detail</details>\n    </errorCode>\n  </subscribeError>\n</updateContextSubscriptionResponse>\n";
  std::string ucsExpected2 = "<updateContextSubscriptionResponse>\n  <subscribeError>\n    <subscriptionId>0</subscriptionId>\n    <errorCode>\n      <code>400</code>\n      <reasonPhrase>Bad Request</reasonPhrase>\n      <details>detail</details>\n    </errorCode>\n  </subscribeError>\n</updateContextSubscriptionResponse>\n";
  
  std::string uscr1 = "unsubscribeContext";
  std::string uscr2 = "/ngsi10/unsubscribeContext";
  std::string uscr3 = "/NGSI10/unsubscribeContext";
  std::string uscr4 = "unsubscribeContextRequest";
  std::string uscrExpected  = "<unsubscribeContextResponse>\n  <subscriptionId>0</subscriptionId>\n  <statusCode>\n    <code>200</code>\n    <reasonPhrase>ok</reasonPhrase>\n    <details>detail</details>\n  </statusCode>\n</unsubscribeContextResponse>\n";
  std::string uscrExpected2 = "<unsubscribeContextResponse>\n  <subscriptionId>0</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </statusCode>\n</unsubscribeContextResponse>\n";
  
  std::string ucr1 = "updateContext";
  std::string ucr2 = "/ngsi10/updateContext";
  std::string ucr3 = "/NGSI10/updateContext";
  std::string ucr4 = "updateContextRequest";
  std::string ucrExpected  = "<updateContextResponse>\n  <errorCode>\n    <code>200</code>\n    <reasonPhrase>ok</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</updateContextResponse>\n";
  std::string ucrExpected2 = "<updateContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </errorCode>\n</updateContextResponse>\n";
  
  std::string ncr1 = "notifyContext";
  std::string ncr2 = "/ngsi10/notifyContext";
  std::string ncr3 = "/NGSI10/notifyContext";
  std::string ncr4 = "notifyContextRequest";
  std::string ncrExpected  = "<notifyContextResponse>\n  <responseCode>\n    <code>200</code>\n    <reasonPhrase>ok</reasonPhrase>\n    <details>detail</details>\n  </responseCode>\n</notifyContextResponse>\n";
  std::string ncrExpected2 = "<notifyContextResponse>\n  <responseCode>\n    <code>400</code>\n    <reasonPhrase>Bad Request</reasonPhrase>\n    <details>detail</details>\n  </responseCode>\n</notifyContextResponse>\n";
  
  std::string     out;
  ConnectionInfo  ci("/ngsi/test", "POST", "1.1");
  
  utInit();

  out = restErrorReplyGet(&ci, XML, "", rcr1, 200, "ok", "detail");
  EXPECT_EQ(out, rcrExpected);
  out = restErrorReplyGet(&ci, XML, "", rcr2, 200, "ok", "detail");
  EXPECT_EQ(out, rcrExpected);
  out = restErrorReplyGet(&ci, XML, "", rcr3, 200, "ok", "detail");
  EXPECT_EQ(out, rcrExpected);
  out = restErrorReplyGet(&ci, XML, "", rcr4, 200, "ok", "detail");
  EXPECT_EQ(out, rcrExpected);

  out = restErrorReplyGet(&ci, XML, "", rcr1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, rcrExpected2);
  out = restErrorReplyGet(&ci, XML, "", rcr2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, rcrExpected2);
  out = restErrorReplyGet(&ci, XML, "", rcr3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, rcrExpected2);
  out = restErrorReplyGet(&ci, XML, "", rcr4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, rcrExpected2);


  out = restErrorReplyGet(&ci, XML, "", dcar1, 200, "ok", "detail");
  EXPECT_EQ(out, dcarExpected);
  out = restErrorReplyGet(&ci, XML, "", dcar2, 200, "ok", "detail");
  EXPECT_EQ(out, dcarExpected);
  out = restErrorReplyGet(&ci, XML, "", dcar3, 200, "ok", "detail");
  EXPECT_EQ(out, dcarExpected);
  out = restErrorReplyGet(&ci, XML, "", dcar4, 200, "ok", "detail");
  EXPECT_EQ(out, dcarExpected);

  out = restErrorReplyGet(&ci, XML, "", dcar1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, dcarExpected2);
  out = restErrorReplyGet(&ci, XML, "", dcar2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, dcarExpected2);
  out = restErrorReplyGet(&ci, XML, "", dcar3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, dcarExpected2);
  out = restErrorReplyGet(&ci, XML, "", dcar4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, dcarExpected2);


  out = restErrorReplyGet(&ci, XML, "", scar1, 200, "ok", "detail");
  EXPECT_EQ(out, scarExpected);
  out = restErrorReplyGet(&ci, XML, "", scar2, 200, "ok", "detail");
  EXPECT_EQ(out, scarExpected);
  out = restErrorReplyGet(&ci, XML, "", scar3, 200, "ok", "detail");
  EXPECT_EQ(out, scarExpected);
  out = restErrorReplyGet(&ci, XML, "", scar4, 200, "ok", "detail");
  EXPECT_EQ(out, scarExpected);

  out = restErrorReplyGet(&ci, XML, "", scar1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, scarExpected2);
  out = restErrorReplyGet(&ci, XML, "", scar2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, scarExpected2);
  out = restErrorReplyGet(&ci, XML, "", scar3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, scarExpected2);
  out = restErrorReplyGet(&ci, XML, "", scar4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, scarExpected2);


  out = restErrorReplyGet(&ci, XML, "", ucas1, 200, "ok", "detail");
  EXPECT_EQ(out, ucasExpected);
  out = restErrorReplyGet(&ci, XML, "", ucas2, 200, "ok", "detail");
  EXPECT_EQ(out, ucasExpected);
  out = restErrorReplyGet(&ci, XML, "", ucas3, 200, "ok", "detail");
  EXPECT_EQ(out, ucasExpected);
  out = restErrorReplyGet(&ci, XML, "", ucas4, 200, "ok", "detail");
  EXPECT_EQ(out, ucasExpected);

  out = restErrorReplyGet(&ci, XML, "", ucas1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucasExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucas2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucasExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucas3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucasExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucas4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucasExpected2);


  out = restErrorReplyGet(&ci, XML, "", ucar1, 200, "ok", "detail");
  EXPECT_EQ(out, ucarExpected);
  out = restErrorReplyGet(&ci, XML, "", ucar2, 200, "ok", "detail");
  EXPECT_EQ(out, ucarExpected);
  out = restErrorReplyGet(&ci, XML, "", ucar3, 200, "ok", "detail");
  EXPECT_EQ(out, ucarExpected);
  out = restErrorReplyGet(&ci, XML, "", ucar4, 200, "ok", "detail");
  EXPECT_EQ(out, ucarExpected);

  out = restErrorReplyGet(&ci, XML, "", ucar1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucarExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucar2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucarExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucar3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucarExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucar4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucarExpected2);


  out = restErrorReplyGet(&ci, XML, "", ncar1, 200, "ok", "detail");
  EXPECT_EQ(out, ncarExpected);
  out = restErrorReplyGet(&ci, XML, "", ncar2, 200, "ok", "detail");
  EXPECT_EQ(out, ncarExpected);
  out = restErrorReplyGet(&ci, XML, "", ncar3, 200, "ok", "detail");
  EXPECT_EQ(out, ncarExpected);
  out = restErrorReplyGet(&ci, XML, "", ncar4, 200, "ok", "detail");
  EXPECT_EQ(out, ncarExpected);

  out = restErrorReplyGet(&ci, XML, "", ncar1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ncarExpected2);
  out = restErrorReplyGet(&ci, XML, "", ncar2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ncarExpected2);
  out = restErrorReplyGet(&ci, XML, "", ncar3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ncarExpected2);
  out = restErrorReplyGet(&ci, XML, "", ncar4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ncarExpected2);


  out = restErrorReplyGet(&ci, XML, "", qcr1, 200, "ok", "detail");
  EXPECT_EQ(out, qcrExpected);
  out = restErrorReplyGet(&ci, XML, "", qcr2, 200, "ok", "detail");
  EXPECT_EQ(out, qcrExpected);
  out = restErrorReplyGet(&ci, XML, "", qcr3, 200, "ok", "detail");
  EXPECT_EQ(out, qcrExpected);
  out = restErrorReplyGet(&ci, XML, "", qcr4, 200, "ok", "detail");
  EXPECT_EQ(out, qcrExpected);

  out = restErrorReplyGet(&ci, XML, "", qcr1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, qcrExpected2);
  out = restErrorReplyGet(&ci, XML, "", qcr2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, qcrExpected2);
  out = restErrorReplyGet(&ci, XML, "", qcr3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, qcrExpected2);
  out = restErrorReplyGet(&ci, XML, "", qcr4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, qcrExpected2);


  out = restErrorReplyGet(&ci, XML, "", scr1, 200, "ok", "detail");
  EXPECT_EQ(out, scrExpected);
  out = restErrorReplyGet(&ci, XML, "", scr2, 200, "ok", "detail");
  EXPECT_EQ(out, scrExpected);
  out = restErrorReplyGet(&ci, XML, "", scr3, 200, "ok", "detail");
  EXPECT_EQ(out, scrExpected);
  out = restErrorReplyGet(&ci, XML, "", scr4, 200, "ok", "detail");
  EXPECT_EQ(out, scrExpected);

  out = restErrorReplyGet(&ci, XML, "", scr1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, scrExpected2);
  out = restErrorReplyGet(&ci, XML, "", scr2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, scrExpected2);
  out = restErrorReplyGet(&ci, XML, "", scr3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, scrExpected2);
  out = restErrorReplyGet(&ci, XML, "", scr4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, scrExpected2);


  out = restErrorReplyGet(&ci, XML, "", ucs1, 200, "ok", "detail");
  EXPECT_EQ(out, ucsExpected);
  out = restErrorReplyGet(&ci, XML, "", ucs2, 200, "ok", "detail");
  EXPECT_EQ(out, ucsExpected);
  out = restErrorReplyGet(&ci, XML, "", ucs3, 200, "ok", "detail");
  EXPECT_EQ(out, ucsExpected);
  out = restErrorReplyGet(&ci, XML, "", ucs4, 200, "ok", "detail");
  EXPECT_EQ(out, ucsExpected);

  out = restErrorReplyGet(&ci, XML, "", ucs1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucsExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucs2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucsExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucs3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucsExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucs4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucsExpected2);


  out = restErrorReplyGet(&ci, XML, "", uscr1, 200, "ok", "detail");
  EXPECT_EQ(out, uscrExpected);
  out = restErrorReplyGet(&ci, XML, "", uscr2, 200, "ok", "detail");
  EXPECT_EQ(out, uscrExpected);
  out = restErrorReplyGet(&ci, XML, "", uscr3, 200, "ok", "detail");
  EXPECT_EQ(out, uscrExpected);
  out = restErrorReplyGet(&ci, XML, "", uscr4, 200, "ok", "detail");
  EXPECT_EQ(out, uscrExpected);

  out = restErrorReplyGet(&ci, XML, "", uscr1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, uscrExpected2);
  out = restErrorReplyGet(&ci, XML, "", uscr2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, uscrExpected2);
  out = restErrorReplyGet(&ci, XML, "", uscr3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, uscrExpected2);
  out = restErrorReplyGet(&ci, XML, "", uscr4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, uscrExpected2);


  out = restErrorReplyGet(&ci, XML, "", ucr1, 200, "ok", "detail");
  EXPECT_EQ(out, ucrExpected);
  out = restErrorReplyGet(&ci, XML, "", ucr2, 200, "ok", "detail");
  EXPECT_EQ(out, ucrExpected);
  out = restErrorReplyGet(&ci, XML, "", ucr3, 200, "ok", "detail");
  EXPECT_EQ(out, ucrExpected);
  out = restErrorReplyGet(&ci, XML, "", ucr4, 200, "ok", "detail");
  EXPECT_EQ(out, ucrExpected);

  out = restErrorReplyGet(&ci, XML, "", ucr1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucrExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucr2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucrExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucr3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucrExpected2);
  out = restErrorReplyGet(&ci, XML, "", ucr4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ucrExpected2);


  out = restErrorReplyGet(&ci, XML, "", ncr1, 200, "ok", "detail");
  EXPECT_EQ(out, ncrExpected);
  out = restErrorReplyGet(&ci, XML, "", ncr2, 200, "ok", "detail");
  EXPECT_EQ(out, ncrExpected);
  out = restErrorReplyGet(&ci, XML, "", ncr3, 200, "ok", "detail");
  EXPECT_EQ(out, ncrExpected);
  out = restErrorReplyGet(&ci, XML, "", ncr4, 200, "ok", "detail");
  EXPECT_EQ(out, ncrExpected);

  out = restErrorReplyGet(&ci, XML, "", ncr1, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ncrExpected2);
  out = restErrorReplyGet(&ci, XML, "", ncr2, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ncrExpected2);
  out = restErrorReplyGet(&ci, XML, "", ncr3, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ncrExpected2);
  out = restErrorReplyGet(&ci, XML, "", ncr4, 400, "Bad Request", "detail");
  EXPECT_EQ(out, ncrExpected2);

  utExit();
}
