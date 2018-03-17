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

#include "ngsi/RestrictionString.h"

#include "unittest.h"


/* ****************************************************************************
*
* check - should RestrictionString::check always return "OK"?
*/
TEST(RestrictionString, check)
{
  RestrictionString   restrictionString;
  std::string         checked;

  utInit();

  checked = restrictionString.check();
  EXPECT_STREQ("OK", checked.c_str());

  restrictionString.string = "String";

  checked = restrictionString.check();
  EXPECT_STREQ("OK", checked.c_str());

  utExit();
}



/* ****************************************************************************
*
* isEmptSetAndGet -
*/
TEST(RestrictionString, isEmptySetAndGet)
{
  RestrictionString   restrictionString;

  utInit();

  restrictionString.string = "";
  EXPECT_TRUE(restrictionString.isEmpty());

  restrictionString.set("STR");
  EXPECT_FALSE(restrictionString.isEmpty());

  EXPECT_STREQ("STR", restrictionString.get().c_str());

  utExit();
}



/* ****************************************************************************
*
* render -
*/
TEST(RestrictionString, render)
{
  RestrictionString   restrictionString;
  std::string         out;
  const char*         outfile1 = "ngsi.restrictionString.render.middle.json";

  utInit();

  out = restrictionString.render(false);
  EXPECT_STREQ("", out.c_str());

  restrictionString.string = "String";

  out = restrictionString.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(RestrictionString, present)
{
  RestrictionString   restrictionString;

  utInit();

  restrictionString.set("");
  restrictionString.present("");

  restrictionString.set("STR");
  restrictionString.present("");

  utExit();
}



/* ****************************************************************************
*
* c_str -
*/
TEST(RestrictionString, c_str)
{
  RestrictionString   restrictionString;

  utInit();

  restrictionString.set("STR");
  EXPECT_STREQ("STR", restrictionString.c_str());

  utExit();
}
