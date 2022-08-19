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
#include "ngsi/ErrorCode.h"

#include "unittest.h"



/* ****************************************************************************
*
* render -
*/
TEST(ErrorCode, render)
{
  ErrorCode    e1;
  std::string  out;
  const char*  outfile1 = "ngsi.errorCode.render1.middle.json";

  utInit();

  out = e1.render();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* check -
*/
TEST(ErrorCode, check)
{
  ErrorCode    e1(0, "REASON", "DETAILS");
  ErrorCode    e2(200, "", "DETAILS");
  ErrorCode    e3(200, "REASON", "DETAILS");
  std::string  rendered;
  std::string  expected1 = "no code";
  std::string  expected2 = "no reason phrase";
  std::string  expected3 = "OK";

  utInit();

  rendered = e1.check(RegisterContext, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  rendered = e2.check(RegisterContext, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = e3.check(RegisterContext, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());

  utExit();
}
