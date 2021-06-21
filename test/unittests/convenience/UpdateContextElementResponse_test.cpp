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

#include "convenience/UpdateContextElementResponse.h"
#include "convenience/ContextAttributeResponseVector.h"
#include "rest/ConnectionInfo.h"

#include "unittest.h"



/* ****************************************************************************
*
* render_json - 
*/
TEST(UpdateContextElementResponse, render_json)
{
  UpdateContextElementResponse    ucer;
  ContextAttributeResponse        car;
  ContextAttribute                ca("caName", "caType", "caValue");
  std::string                     out;
  const char*                     outfile = "ngsi10.updateContextElementResponse.ok.valid.json";

  // Just the normal case
  ucer.contextAttributeResponseVector.push_back(&car);
  car.contextAttributeVector.push_back(&ca);
  car.statusCode.fill(SccOk, "details");

  out = ucer.toJsonV1(false, UpdateContext);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());
}



/* ****************************************************************************
*
* check_json - 
*/
TEST(UpdateContextElementResponse, check_json)
{
  UpdateContextElementResponse  ucer;
  ContextAttributeResponse      car;
  ContextAttribute              ca("", "TYPE", "VALUE"); // empty name, thus provoking error
  std::string                   out;
  const char*                   outfile1 = "ngsi10.updateContextElementResponse.check1.valid.json";
  const char*                   outfile2 = "ngsi10.updateContextElementResponse.check2.valid.json";

  // 1. predetected error
  out = ucer.check(V1, false, IndividualContextEntity, "PRE ERR");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  // 2. bad contextAttributeResponseVector
  car.contextAttributeVector.push_back(&ca);
  ucer.contextAttributeResponseVector.push_back(&car);
  out = ucer.check(V1, false, IndividualContextEntity, "");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  // 3. OK
  ca.name = "NAME";
  out = ucer.check(V1, false, IndividualContextEntity, "");
  EXPECT_EQ("OK", out);
}
