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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/StatusCode.h"

#include "unittest.h"



/* ****************************************************************************
*
* render - 
*/
TEST(StatusCode, render)
{
  StatusCode    sc1;
  StatusCode    sc2(SccOk, "DETAILS");
  std::string   out;
  const char*   outfile1  = "ngsi.statusCode.render4.middle.json";

  utInit();

  out = sc2.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  sc1.release(); // just to exercise the code ...

  utExit();
}



/* ****************************************************************************
*
* fill - 
*/
TEST(StatusCode, fill)
{
  StatusCode    sc;
  StatusCode    sc2(SccOk, "Details");
  StatusCode    ec(SccBadRequest, "Very bad request :-)");
  std::string   out;

  utInit();

  sc.fill(SccForbidden, "D");
  EXPECT_EQ(sc.code, SccForbidden);
  EXPECT_STREQ(sc.reasonPhrase.c_str(), "Forbidden");
  EXPECT_STREQ(sc.details.c_str(), "D");

  sc.fill(&sc2);
  EXPECT_EQ(sc.code, SccOk);
  EXPECT_STREQ(sc.reasonPhrase.c_str(), "OK");
  EXPECT_STREQ(sc.details.c_str(), "Details");

  sc.fill(&ec);
  EXPECT_EQ(sc.code, SccBadRequest);
  EXPECT_STREQ(sc.reasonPhrase.c_str(), "Bad Request");
  EXPECT_STREQ(sc.details.c_str(), "Very bad request :-)");

  utExit();
}



/* ****************************************************************************
*
* check - 
*/
TEST(StatusCode, check)
{
  StatusCode    sc(SccOk, "");
  std::string   out;

  utInit();

  out = sc.check();
  EXPECT_STREQ("OK", out.c_str());

  sc.fill(SccNone, "YYY");
  out = sc.check();
  EXPECT_STREQ("no code", out.c_str());

  sc.fill(SccOk, "YYY");
  out = sc.check();
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(StatusCode, present)
{
  StatusCode    sc(SccOk, "");
  std::string   out;

  utInit();
  sc.present("");
  utExit();
}
