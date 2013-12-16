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

#include "ngsi/NotifyCondition.h"



/* ****************************************************************************
*
* Creation - 
*/
TEST(NotifyCondition, Creation)
{
  NotifyCondition nc;

  nc.restriction.set("Hola");

  EXPECT_TRUE(nc.restriction.get() == "Hola");
}



/* ****************************************************************************
*
* render - 
*/
TEST(NotifyCondition, render)
{
  NotifyCondition  nc;
  std::string      expected1xml  = "<notifyCondition>\n  <type></type>\n</notifyCondition>\n";
  std::string      expected1json = "{\n  \"type\" : \"\"\n}\n";
  std::string      rendered;

  rendered = nc.render(XML, "", false);
  EXPECT_STREQ(expected1xml.c_str(), rendered.c_str());
  rendered = nc.render(JSON, "", false);
  EXPECT_STREQ(expected1json.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(NotifyCondition, present)
{
  NotifyCondition  nc;

  nc.present("", -1);
  nc.present("", 0);
}



/* ****************************************************************************
*
* check - 
*/
TEST(NotifyCondition, check)
{
  NotifyCondition  nc;
  std::string      expected1 = "empty type for NotifyCondition";
  std::string      expected2 = "invalid notify condition type: 'XXX'";
  std::string      checked;

  checked = nc.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), checked.c_str());
  
  nc.type = "XXX";
  checked = nc.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  nc.release();
}
