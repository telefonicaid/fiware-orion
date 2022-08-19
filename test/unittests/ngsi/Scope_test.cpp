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

#include "ngsi/Scope.h"

#include "unittest.h"



/* ****************************************************************************
*
* render -
*/
TEST(Scope, render)
{
  Scope        scope("Type", "Value");
  std::string  out;
  const char*  outfile1 = "ngsi.scope.render.middle.json";

  utInit();

  out = scope.toJsonV1(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  scope.release();

  utExit();
}



/* ****************************************************************************
*
* check - should Scope::check always return "OK"?
*/
TEST(Scope, check)
{
  Scope        scope;
  Scope        scope1("",     "value");
  Scope        scope2("type", "");
  Scope        scope3("type", "value");
  std::string  checked;
  std::string  expected  = "Empty type in restriction scope";
  std::string  expected1 = "Empty type in restriction scope";
  std::string  expected2 = "Empty value in restriction scope";
  std::string  expected3 = "OK";

  utInit();

  checked = scope.check();
  EXPECT_STREQ(checked.c_str(), expected.c_str());

  checked = scope1.check();
  EXPECT_STREQ(checked.c_str(), expected1.c_str());

  checked = scope2.check();
  EXPECT_STREQ(checked.c_str(), expected2.c_str());

  checked = scope3.check();
  EXPECT_STREQ(checked.c_str(), expected3.c_str());

  utExit();
}
