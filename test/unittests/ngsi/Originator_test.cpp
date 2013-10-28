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

#include "ngsi/Originator.h"



/* ****************************************************************************
*
* check - should Originator::check always return "OK"?
*/
TEST(Originator, check)
{
  Originator   originator;
  std::string  checked;
  std::string  expected1 = "OK";
  std::string  expected2 = "OK";
  std::string  expected3 = "OK";

  checked = originator.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), checked.c_str());

  originator.string = "String";

  checked = originator.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  checked = originator.check(RegisterContext, JSON, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), checked.c_str());
}



/* ****************************************************************************
*
* isEmptSetAndGet - 
*/
TEST(Originator, isEmptySetAndGet)
{
  Originator   originator;

  originator.string = "";
  EXPECT_TRUE(originator.isEmpty());

  originator.set("STR");
  EXPECT_FALSE(originator.isEmpty());

  EXPECT_STREQ("STR", originator.get().c_str());
}



/* ****************************************************************************
*
* render - 
*/
TEST(Originator, render)
{
  Originator   originator;
  std::string  rendered;
  std::string  expected1 = "";
  std::string  expected2 = "<originator>String</originator>\n";
  std::string  expected3 = "\"originator\" : \"String\"\n";

  rendered = originator.render(XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  originator.string = "String";

  rendered = originator.render(XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = originator.render(JSON, "");
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Originator, present)
{
  Originator   originator;

  originator.set("");
  originator.present("");

  originator.set("STR");
  originator.present("");
}



/* ****************************************************************************
*
* c_str - 
*/
TEST(Originator, c_str)
{
  Originator   originator;

  originator.set("STR");
  EXPECT_STREQ("STR", originator.c_str());
}
