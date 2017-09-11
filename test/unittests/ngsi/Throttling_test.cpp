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

#include "ngsi/Throttling.h"

#include "unittest.h"



/* ****************************************************************************
*
* setGetIsEmptyAndParse - 
*/
TEST(Throttling, setGetIsEmptyAndParse)
{
  Throttling  t;

  utInit();

  t.set("PT5S");
  EXPECT_STREQ("PT5S", t.get().c_str());
  EXPECT_EQ(5, t.parse());
  EXPECT_FALSE(t.isEmpty());

  t.set("");
  EXPECT_TRUE(t.isEmpty());

  utExit();
}



/* ****************************************************************************
*
* check - 
*/
TEST(Throttling, check)
{
  Throttling   t;
  std::string  checked;

  utInit();

  t.set("");
  checked = t.check(RegisterContext, "", "", 0);
  EXPECT_EQ("OK", checked);

  t.set("PT5S");
  checked = t.check(RegisterContext, "", "", 0);
  EXPECT_EQ("OK", checked);

  t.set("xxxPT5S");
  checked = t.check(RegisterContext, "", "", 0);
  EXPECT_EQ("syntax error in throttling string", checked);

  utExit();
}



/* ****************************************************************************
*
* render - 
*/
TEST(Throttling, render)
{
  Throttling   t;
  const char*  outfile1 = "ngsi.throttling.render.middle.json";

  utInit();

  {
    JsonHelper writer(2);
    t.set("");
    t.toJson(writer);
    EXPECT_STREQ("", writer.str().c_str());
  }

  {
    JsonHelper writer(2);
    t.toJson(writer);
    EXPECT_STREQ("", writer.str().c_str());
  }

  t.set("PT1S");

  {
    JsonHelper writer(2);
    writer.StartObject();
    t.toJson(writer);
    writer.EndObject();
    EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
    EXPECT_STREQ(expectedBuf, writer.str().c_str());
  }

  utExit();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Throttling, present)
{
  Throttling   t;

  utInit();

  t.set("PT1S");
  t.present("");

  t.set("");
  t.present("");

  utExit();
}
