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
#include "convenience/ContextAttributeResponse.h"
#include "rest/ConnectionInfo.h"

#include "unittest.h"



/* ****************************************************************************
*
* render_json -
*/
TEST(ContextAttributeResponse, render_json)
{
  ContextAttribute          ca("caName", "caType", "caValue");
  ContextAttributeResponse  car;
  std::string               out;

  utInit();

  car.contextAttributeVector.push_back(&ca);
  car.statusCode.fill(SccOk, "OK");

  out = car.render(V1, false, ContextEntityAttributes);

  utExit();
}



/* ****************************************************************************
*
* check_json -
*/
TEST(ContextAttributeResponse, check_json)
{
  ContextAttribute          ca("caName", "caType", "caValue");
  ContextAttributeResponse  car;
  std::string               out;
  const char*               outfile1 = "ngsi10.contextAttributeResponse.check3.valid.json";
  const char*               outfile2 = "ngsi10.contextAttributeResponse.check4.valid.json";

  utInit();

  // 1. OK
  car.contextAttributeVector.push_back(&ca);
  car.statusCode.fill(SccOk, "OK");

  out = car.check(V1, false, UpdateContextAttribute, "");
  EXPECT_STREQ("OK", out.c_str());


  // 2. predetectedError
  out = car.check(V1, false, UpdateContextAttribute, "PRE Error");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());


  // 3. Bad ContextAttribute
  ContextAttribute          ca2("", "caType", "caValue");
  car.contextAttributeVector.push_back(&ca2);

  LM_M(("car.contextAttributeVector.size: %d - calling ContextAttributeResponse::check", car.contextAttributeVector.size()));
  out = car.check(V1, false, UpdateContextAttribute, "");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - just exercise the code
*/
TEST(ContextAttributeResponse, present)
{
  ContextAttribute          ca("caName", "caType", "caValue");
  ContextAttributeResponse  car;

  utInit();

  car.contextAttributeVector.push_back(&ca);
  car.statusCode.fill(SccOk);

  car.present("");

  utExit();
}



/* ****************************************************************************
*
* release - just exercise the code
*/
TEST(ContextAttributeResponse, release)
{
  ContextAttribute*         caP = new ContextAttribute("caName", "caType", "caValue");
  ContextAttributeResponse  car;

  utInit();

  car.contextAttributeVector.push_back(caP);
  car.statusCode.fill(SccOk);

  car.release();

  utExit();
}
