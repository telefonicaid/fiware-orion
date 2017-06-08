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

#include "ngsi/Originator.h"

#include "unittest.h"



/* ****************************************************************************
*
* check - should Originator::check always return "OK"?
*/
TEST(Originator, check)
{
  Originator   originator;
  std::string  checked;

  utInit();

  checked = originator.check();
  EXPECT_STREQ("OK", checked.c_str());

  originator.string = "String";

  checked = originator.check();
  EXPECT_STREQ("OK", checked.c_str());

  checked = originator.check();
  EXPECT_STREQ("OK", checked.c_str());

  utExit();
}



/* ****************************************************************************
*
* isEmptSetAndGet -
*/
TEST(Originator, isEmptySetAndGet)
{
  Originator   originator;

  utInit();

  originator.string = "";
  EXPECT_TRUE(originator.isEmpty());

  originator.set("STR");
  EXPECT_FALSE(originator.isEmpty());

  EXPECT_STREQ("STR", originator.get().c_str());

  utExit();
}



/* ****************************************************************************
*
* render -
*/
TEST(Originator, render)
{
  Originator   originator;
  std::string  out;
  const char*  outfile1 = "ngsi.originator.render.middle.json";

  utInit();

  out = originator.render(false);
  EXPECT_STREQ("", out.c_str());

  originator.string = "String";

  out = originator.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Originator, present)
{
  Originator   originator;

  utInit();

  originator.set("");
  originator.present("");

  originator.set("STR");
  originator.present("");

  utExit();
}



/* ****************************************************************************
*
* c_str -
*/
TEST(Originator, c_str)
{
  Originator   originator;

  utInit();

  originator.set("STR");
  EXPECT_STREQ("STR", originator.c_str());

  utExit();
}
