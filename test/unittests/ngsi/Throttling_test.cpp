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

#include "ngsi/Throttling.h"



/* ****************************************************************************
*
* setGetIsEmptyAndParse - 
*/
TEST(Throttling, setGetIsEmptyAndParse)
{
  Throttling  t;

  t.set("PT5S");
  EXPECT_STREQ("PT5S", t.get().c_str());
  EXPECT_EQ(5, t.parse());
  EXPECT_FALSE(t.isEmpty());

  t.set("");
  EXPECT_TRUE(t.isEmpty());
}



/* ****************************************************************************
*
* check - 
*/
TEST(Throttling, check)
{
  Throttling   t;
  std::string  checked;
  std::string  expected1 = "OK";
  std::string  expected2 = "OK";
  std::string  expected3 = "syntax error in throttling string";

  t.set("");
  checked = t.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ(expected1, checked);

  t.set("PT5S");
  checked = t.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ(expected2, checked);

  t.set("xxxPT5S");
  checked = t.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ(expected3, checked);
}



/* ****************************************************************************
*
* render - 
*/
TEST(Throttling, render)
{
  Throttling   t;
  std::string  rendered;
  std::string  expected1 = "";
  std::string  expected2 = "<throttling>PT1S</throttling>\n";
  std::string  expected3 = "\"throttling\" : \"PT1S\"\n";

  t.set("");
  rendered = t.render(XML, "", false);
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  t.set("PT1S");
  rendered = t.render(XML, "", false);
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = t.render(JSON, "", false);
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Throttling, present)
{
  Throttling   t;

  t.set("PT1S");
  t.present("");

  t.set("");
  t.present("");
}
