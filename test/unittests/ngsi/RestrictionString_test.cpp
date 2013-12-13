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

#include "ngsi/RestrictionString.h"



/* ****************************************************************************
*
* check - should RestrictionString::check always return "OK"?
*/
TEST(RestrictionString, check)
{
  RestrictionString   restrictionString;
  std::string  checked;
  std::string  expected1 = "OK";
  std::string  expected2 = "OK";
  std::string  expected3 = "OK";

  checked = restrictionString.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), checked.c_str());

  restrictionString.string = "String";

  checked = restrictionString.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  checked = restrictionString.check(RegisterContext, JSON, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), checked.c_str());
}



/* ****************************************************************************
*
* isEmptSetAndGet - 
*/
TEST(RestrictionString, isEmptySetAndGet)
{
  RestrictionString   restrictionString;

  restrictionString.string = "";
  EXPECT_TRUE(restrictionString.isEmpty());

  restrictionString.set("STR");
  EXPECT_FALSE(restrictionString.isEmpty());

  EXPECT_STREQ("STR", restrictionString.get().c_str());
}



/* ****************************************************************************
*
* render - 
*/
TEST(RestrictionString, render)
{
  RestrictionString   restrictionString;
  std::string  rendered;
  std::string  expected1 = "";
  std::string  expected2 = "<restriction>String</restriction>\n";
  std::string  expected3 = "\"restriction\" : \"String\"\n";

  rendered = restrictionString.render(XML, "", false);
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  restrictionString.string = "String";

  rendered = restrictionString.render(XML, "", false);
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = restrictionString.render(JSON, "", false);
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(RestrictionString, present)
{
  RestrictionString   restrictionString;

  restrictionString.set("");
  restrictionString.present("");

  restrictionString.set("STR");
  restrictionString.present("");
}



/* ****************************************************************************
*
* c_str - 
*/
TEST(RestrictionString, c_str)
{
  RestrictionString   restrictionString;

  restrictionString.set("STR");
  EXPECT_STREQ("STR", restrictionString.c_str());
}
