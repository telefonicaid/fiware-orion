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
* MHD_create_response_from_data_error - 
*
* Too large response string 
*/
#define TEST_SIZE (4 * 1024 * 1024)
TEST(restReply, MHD_create_response_from_data_error)
{
  ConnectionInfo  ci("/ngsi/XXX", "GET", "1.1");
  char*           answer = (char*) malloc(TEST_SIZE);

  utInit();

  if (answer != NULL)
  {
    memset(answer, 'x', TEST_SIZE - 1);
    answer[TEST_SIZE - 1] = 0;

    restReply(&ci, answer);
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

  utInit();

  ci.outMimeType = JSON;
  restReply(&ci, "123");

  utExit();
}



/* ****************************************************************************
*
* restErrorReplyGet - 
*
* FIXME P5 #1862: _json counterpart?
*/
TEST(restReply, DISABLED_restErrorReplyGet)
{
  const char* rcrOutfile01   = "ngsi9.restReply.registerContext01.ok.valid.xml";
  const char* rcrOutfile02   = "ngsi9.restReply.registerContext02.ok.valid.xml";
  const char* dcarOutfile01  = "ngsi9.restReply.discovery01.ok.valid.xml";
  const char* dcarOutfile02  = "ngsi9.restReply.discovery02.ok.valid.xml";
  const char* scarOutfile01  = "ngsi9.restReply.subscribeContextAvailability01.ok.valid.xml";
  const char* scarOutfile02  = "ngsi9.restReply.subscribeContextAvailability02.ok.valid.xml";
  const char* ucasOutfile01  = "ngsi9.restReply.updateContextAvailabilitySubscription01.ok.valid.xml";
  const char* ucasOutfile02  = "ngsi9.restReply.updateContextAvailabilitySubscription02.ok.valid.xml";
  const char* ucarOutfile01  = "ngsi9.restReply.unsubscribeContextAvailability01.ok.valid.xml";
  const char* ucarOutfile02  = "ngsi9.restReply.unsubscribeContextAvailability02.ok.valid.xml";
  const char* ncarOutfile01  = "ngsi9.restReply.notifyContextAvailabilityRequest01.ok.valid.xml";
  const char* ncarOutfile02  = "ngsi9.restReply.notifyContextAvailabilityRequest02.ok.valid.xml";
  const char* qcrOutfile01   = "ngsi10.restReply.queryContextResponse01.ok.valid.xml";
  const char* qcrOutfile02   = "ngsi10.restReply.queryContextResponse02.ok.valid.xml";
  const char* scrOutfile01   = "ngsi10.restReply.subscribeContextResponse01.ok.valid.xml";
  const char* scrOutfile02   = "ngsi10.restReply.subscribeContextResponse02.ok.valid.xml";
  const char* ucsOutfile01   = "ngsi10.restReply.updateContextSubscriptionResponse01.ok.valid.xml";
  const char* ucsOutfile02   = "ngsi10.restReply.updateContextSubscriptionResponse02.ok.valid.xml";
  const char* uscrOutfile01  = "ngsi10.restReply.unsubscribeContextResponse01.ok.valid.xml";
  const char* uscrOutfile02  = "ngsi10.restReply.unsubscribeContextResponse02.ok.valid.xml";
  const char* ucrOutfile01   = "ngsi10.restReply.updateContextResponse01.ok.valid.xml";
  const char* ucrOutfile02   = "ngsi10.restReply.updateContextResponse02.ok.valid.xml";
  const char* ncrOutfile01   = "ngsi10.restReply.notifyContextResponse01.ok.valid.xml";
  const char* ncrOutfile02   = "ngsi10.restReply.notifyContextResponse02.ok.valid.xml";

  std::string rcr1 = "registerContext";
  std::string rcr2 = "/ngsi9/registerContext";
  std::string rcr3 = "/NGSI9/registerContext";
  std::string rcr4 = "registerContextRequest";

  std::string dcar1 = "discoverContextAvailability";
  std::string dcar2 = "/ngsi9/discoverContextAvailability";
  std::string dcar3 = "/NGSI9/discoverContextAvailability";
  std::string dcar4 = "discoverContextAvailabilityRequest";

  std::string scar1 = "subscribeContextAvailability";
  std::string scar2 = "/ngsi9/subscribeContextAvailability";
  std::string scar3 = "/NGSI9/subscribeContextAvailability";
  std::string scar4 = "subscribeContextAvailabilityRequest";

  std::string ucas1 = "updateContextAvailabilitySubscription";
  std::string ucas2 = "/ngsi9/updateContextAvailabilitySubscription";
  std::string ucas3 = "/NGSI9/updateContextAvailabilitySubscription";
  std::string ucas4 = "updateContextAvailabilitySubscriptionRequest";
  
  std::string ucar1 = "unsubscribeContextAvailability";
  std::string ucar2 = "/ngsi9/unsubscribeContextAvailability";
  std::string ucar3 = "/NGSI9/unsubscribeContextAvailability";
  std::string ucar4 = "unsubscribeContextAvailabilityRequest";
  
  std::string ncar1 = "notifyContextAvailability";
  std::string ncar2 = "/ngsi9/notifyContextAvailability";
  std::string ncar3 = "/NGSI9/notifyContextAvailability";
  std::string ncar4 = "notifyContextAvailabilityRequest";
  
  std::string qcr1 = "queryContext";
  std::string qcr2 = "/ngsi10/queryContext";
  std::string qcr3 = "/NGSI10/queryContext";
  std::string qcr4 = "queryContextRequest";
  
  std::string scr1 = "subscribeContext";
  std::string scr2 = "/ngsi10/subscribeContext";
  std::string scr3 = "/NGSI10/subscribeContext";
  std::string scr4 = "subscribeContextRequest";
  
  std::string ucs1 = "updateContextSubscription";
  std::string ucs2 = "/ngsi10/updateContextSubscription";
  std::string ucs3 = "/NGSI10/updateContextSubscription";
  std::string ucs4 = "updateContextSubscriptionRequest";
  
  std::string uscr1 = "unsubscribeContext";
  std::string uscr2 = "/ngsi10/unsubscribeContext";
  std::string uscr3 = "/NGSI10/unsubscribeContext";
  std::string uscr4 = "unsubscribeContextRequest";
  
  std::string ucr1 = "updateContext";
  std::string ucr2 = "/ngsi10/updateContext";
  std::string ucr3 = "/NGSI10/updateContext";
  std::string ucr4 = "updateContextRequest";
  
  std::string ncr1 = "notifyContext";
  std::string ncr2 = "/ngsi10/notifyContext";
  std::string ncr3 = "/NGSI10/notifyContext";
  std::string ncr4 = "notifyContextRequest";
  
  std::string     out;
  ConnectionInfo  ci("/ngsi/test", "POST", "1.1");
  
  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), rcrOutfile01)) << "Error getting test data from '" << rcrOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", rcr1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", rcr2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", rcr3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", rcr4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), rcrOutfile02)) << "Error getting test data from '" << rcrOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", rcr1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", rcr2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", rcr3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", rcr4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), dcarOutfile01)) << "Error getting test data from '" << dcarOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", dcar1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", dcar2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", dcar3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", dcar4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), dcarOutfile02)) << "Error getting test data from '" << dcarOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", dcar1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", dcar2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", dcar3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", dcar4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), scarOutfile01)) << "Error getting test data from '" << scarOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", scar1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scar2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scar3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scar4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), scarOutfile02)) << "Error getting test data from '" << scarOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", scar1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scar2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scar3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scar4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ucasOutfile01)) << "Error getting test data from '" << ucasOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", ucas1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucas2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucas3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucas4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ucasOutfile02)) << "Error getting test data from '" << ucasOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", ucas1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucas2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucas3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucas4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ucarOutfile01)) << "Error getting test data from '" << ucarOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", ucar1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucar2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucar3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucar4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ucarOutfile02)) << "Error getting test data from '" << ucarOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", ucar1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucar2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucar3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucar4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ncarOutfile01)) << "Error getting test data from '" << ncarOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", ncar1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncar2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncar3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncar4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ncarOutfile02)) << "Error getting test data from '" << ncarOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", ncar1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncar2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncar3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncar4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), qcrOutfile01)) << "Error getting test data from '" << qcrOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", qcr1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", qcr2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", qcr3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", qcr4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), qcrOutfile02)) << "Error getting test data from '" << qcrOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", qcr1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", qcr2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", qcr3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", qcr4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), scrOutfile01)) << "Error getting test data from '" << scrOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", scr1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scr2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scr3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scr4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), scrOutfile02)) << "Error getting test data from '" << scrOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", scr1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scr2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scr3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", scr4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ucsOutfile01)) << "Error getting test data from '" << ucsOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", ucs1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucs2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucs3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucs4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ucsOutfile02)) << "Error getting test data from '" << ucsOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", ucs1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucs2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucs3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucs4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), uscrOutfile01)) << "Error getting test data from '" << uscrOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", uscr1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", uscr2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", uscr3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", uscr4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), uscrOutfile02)) << "Error getting test data from '" << uscrOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", uscr1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", uscr2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", uscr3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", uscr4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ucrOutfile01)) << "Error getting test data from '" << ucrOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", ucr1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = restErrorReplyGet(&ci, "", ucr2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucr3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucr4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ucrOutfile02)) << "Error getting test data from '" << ucrOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", ucr1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucr2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucr3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ucr4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());


  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ncrOutfile01)) << "Error getting test data from '" << ncrOutfile01 << "'";
  out = restErrorReplyGet(&ci, "", ncr1, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncr2, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncr3, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncr4, SccOk, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), ncrOutfile02)) << "Error getting test data from '" << ncrOutfile02 << "'";
  out = restErrorReplyGet(&ci, "", ncr1, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncr2, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncr3, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());
  out = restErrorReplyGet(&ci, "", ncr4, SccBadRequest, "detail");
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
