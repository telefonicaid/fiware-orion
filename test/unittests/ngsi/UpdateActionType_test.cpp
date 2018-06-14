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

#include "ngsi/UpdateActionType.h"

#include "unittest.h"



/* ****************************************************************************
*
* setGetIsEmptyAndParse - 
*/
TEST(UpdateActionType, setGetIsEmptyAndParse)
{
  UpdateActionType  t;

  utInit();

  t.set("Append");
  EXPECT_STREQ("Append", t.get().c_str());
  EXPECT_FALSE(t.isEmpty());

  t.set("");
  EXPECT_TRUE(t.isEmpty());

  utExit();
}



/* ****************************************************************************
*
* check - 
*/
TEST(UpdateActionType, check)
{
  UpdateActionType   uat;
  std::string        checked;
  std::string        expected1 = "OK";
  std::string        expected2 = "OK";
  std::string        expected3 = "invalid update action type: right ones are: APPEND, APPEND_STRICT, DELETE, REPLACE, UPDATE";
  std::string        expected4 = "empty update action type";

  utInit();

  uat.set("Append");
  checked = uat.check();
  EXPECT_STREQ(expected1.c_str(), checked.c_str());

  uat.set("Update");
  checked = uat.check();
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  uat.set("APPEND");
  checked = uat.check();
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  uat.set("Delete");
  checked = uat.check();
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  uat.set("APPEND2");
  checked = uat.check();
  EXPECT_STREQ(expected3.c_str(), checked.c_str());

  uat.set("");
  checked = uat.check();
  EXPECT_STREQ(expected4.c_str(), checked.c_str());

  utExit();
}



/* ****************************************************************************
*
* render - 
*/
TEST(UpdateActionType, render)
{
  UpdateActionType  uat;
  std::string       out;
  const char*       outfile1 = "ngsi.updateActionType.render.middle.json";

  utInit();

  uat.set("");
  out = uat.render(false);
  EXPECT_STREQ("", out.c_str());

  uat.set("Update");
  out = uat.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
