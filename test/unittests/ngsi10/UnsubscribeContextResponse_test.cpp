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
#include "ngsi10/UnsubscribeContextResponse.h"
#include "ngsi/StatusCode.h"

#include "unittest.h"



/* ****************************************************************************
*
* constructorsAndRender -
*
*/
TEST(UnsubscribeContextResponse, constructorsAndRender)
{
  UnsubscribeContextResponse  uncr1;
  StatusCode                  sc(SccOk, "D");
  UnsubscribeContextResponse  uncr2(sc);
  StatusCode                  ec(SccBadRequest, "D");
  UnsubscribeContextResponse  uncr3(ec);
  std::string                 out;

  utInit();

  EXPECT_EQ(0,             uncr1.statusCode.code);
  EXPECT_EQ(SccOk,         uncr2.statusCode.code);
  EXPECT_EQ(SccBadRequest, uncr3.statusCode.code);

  uncr1.release();

  utExit();
}



/* ****************************************************************************
*
* jsonRender -
*/
TEST(UnsubscribeContextResponse, jsonRender)
{
  const char*                  infile1  = "ngsi10.unsubscribeContextResponse.jsonRender1.valid.json";
  const char*                  infile2  = "ngsi10.unsubscribeContextResponse.jsonRender2.valid.json";
  UnsubscribeContextResponse*  uncrP;
  std::string                  out;

  utInit();

  // Preparations
  uncrP = new UnsubscribeContextResponse();

  // 1. 400, with details
  uncrP->subscriptionId.set("012345678901234567890123");
  uncrP->statusCode.fill(SccBadRequest, "details");

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), infile1)) << "Error getting test data from '" << infile1 << "'";
  out = uncrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());


  // 2. 200, no details
  uncrP->subscriptionId.set("012345678901234567890123");
  uncrP->statusCode.fill(SccOk);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), infile2)) << "Error getting test data from '" << infile2 << "'";
  out = uncrP->render();
  EXPECT_STREQ(expectedBuf, out.c_str());

  delete uncrP;

  utExit();
}
