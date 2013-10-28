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

#include "ngsi/UpdateActionType.h"



/* ****************************************************************************
*
* setGetIsEmptyAndParse - 
*/
TEST(UpdateActionType, setGetIsEmptyAndParse)
{
  UpdateActionType  t;

  t.set("Append");
  EXPECT_STREQ("Append", t.get().c_str());
  EXPECT_FALSE(t.isEmpty());

  t.set("");
  EXPECT_TRUE(t.isEmpty());
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
  std::string        expected3 = "bad update action type: 'APPEND2'";

  uat.set("Append");
  checked = uat.check(UpdateContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), checked.c_str());

  uat.set("Update");
  checked = uat.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  uat.set("APPEND");
  checked = uat.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  uat.set("Delete");
  checked = uat.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  uat.set("APPEND2");
  checked = uat.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), checked.c_str());
}



/* ****************************************************************************
*
* render - 
*/
TEST(UpdateActionType, render)
{
  UpdateActionType  uat;
  std::string       rendered;
  std::string       expected1 = "";
  std::string       expected2 = "<updateAction>Update</updateAction>\n";
  std::string       expected3 = "\"updateAction\" : \"Update\"\n";

  uat.set("");
  rendered = uat.render(XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  uat.set("Update");
  rendered = uat.render(XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = uat.render(JSON, "");
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(UpdateActionType, present)
{
  UpdateActionType   uat;

  uat.set("Append");
  uat.present("");

  uat.set("");
  uat.present("");
}
