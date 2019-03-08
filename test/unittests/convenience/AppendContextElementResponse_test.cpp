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

#include "common/MimeType.h"
#include "convenience/AppendContextElementResponse.h"
#include "rest/ConnectionInfo.h"

#include "unittest.h"



/* ****************************************************************************
*
* render_json -
*/
TEST(AppendContextElementResponse, render_json)
{
  AppendContextElementResponse  acer;
  ContextAttributeResponse      car;
  std::string                   out;
  const char*                   outfile1 = "ngsi10.appendContextElementResponse.empty.valid.json";
  const char*                   outfile2 = "ngsi10.appendContextElementResponse.badRequest.valid.json";

  utInit();

  // 1. empty acer
  out = acer.toJsonV1(false, AppendContextElement);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  // 2. errorCode 'active'
  acer.errorCode.fill(SccBadRequest, "very bad request");
  out = acer.toJsonV1(false, AppendContextElement);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* check_json -
*/
TEST(AppendContextElementResponse, check_json)
{
  AppendContextElementResponse  acer;
  ContextAttributeResponse      car;
  ContextAttribute              ca("", "TYPE", "VALUE"); // empty name, thus provoking error
  std::string                   out;
  const char*                   outfile1 = "ngsi10.appendContextElementRequest.check1.postponed.json";
  const char*                   outfile2 = "ngsi10.appendContextElementRequest.check2.postponed.json";

  utInit();

  // 1. predetected error
  out = acer.check(V1, false, IndividualContextEntity, "PRE ERR");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  // 2. bad contextAttributeResponseVector
  car.contextAttributeVector.push_back(&ca);
  acer.contextAttributeResponseVector.push_back(&car);
  out = acer.check(V1, false, IndividualContextEntity, "");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  // 3. OK
  ca.name = "NAME";
  out = acer.check(V1, false, IndividualContextEntity, "");
  EXPECT_EQ("OK", out);

  utExit();
}



/* ****************************************************************************
*
* release -
*/
TEST(AppendContextElementResponse, release)
{
  AppendContextElementResponse  acer;
  ContextAttributeResponse*     carP = new ContextAttributeResponse();
  ContextAttribute*             caP  = new ContextAttribute("NAME", "TYPE", "VALUE");

  utInit();

  carP->contextAttributeVector.push_back(caP);
  acer.contextAttributeResponseVector.push_back(carP);

  EXPECT_EQ(1, carP->contextAttributeVector.size());
  EXPECT_EQ(1, acer.contextAttributeResponseVector.size());
  acer.release();
  EXPECT_EQ(0, acer.contextAttributeResponseVector.size());

  utExit();
}
