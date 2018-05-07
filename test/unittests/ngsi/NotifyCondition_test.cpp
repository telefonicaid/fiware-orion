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

#include "ngsi/NotifyCondition.h"

#include "unittest.h"



/* ****************************************************************************
*
* Creation -
*/
TEST(NotifyCondition, Creation)
{
  NotifyCondition nc;

  utInit();

  nc.restriction.set("Hola");
  EXPECT_TRUE(nc.restriction.get() == "Hola");

  utExit();
}



/* ****************************************************************************
*
* render -
*/
TEST(NotifyCondition, render)
{
  NotifyCondition  nc;
  const char*      outfile1 = "ngsi.notifyCondition.render.middle.json";
  std::string      out;

  utInit();

  out = nc.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(NotifyCondition, present)
{
  NotifyCondition  nc;

  utInit();

  nc.present("", -1);
  nc.present("", 0);

  utExit();
}



/* ****************************************************************************
*
* check -
*/
TEST(NotifyCondition, check)
{
  NotifyCondition  nc;
  std::string      checked;

  utInit();

  checked = nc.check(RegisterContext, "", 0);
  EXPECT_STREQ("empty type for NotifyCondition", checked.c_str());

  nc.type = "XXX";
  checked = nc.check(RegisterContext, "", 0);
  EXPECT_STREQ("invalid notify condition type: /XXX/", checked.c_str());

  nc.release();

  utExit();
}
