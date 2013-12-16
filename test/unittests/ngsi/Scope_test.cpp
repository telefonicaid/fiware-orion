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

#include "ngsi/Scope.h"



/* ****************************************************************************
*
* render - 
*/
TEST(Scope, render)
{
  Scope        scope("Type", "Value");
  std::string  rendered;
  std::string  expected1xml  = "<operationScope>\n  <type>Type</type>\n  <value>Value</value>\n</operationScope>\n";
  std::string  expected1json = "{\n  \"type\" : \"Type\",\n  \"value\" : \"Value\"\n}\n";

  rendered = scope.render(XML, "", false);
  EXPECT_STREQ(expected1xml.c_str(), rendered.c_str());

  rendered = scope.render(JSON, "", false);
  EXPECT_STREQ(expected1json.c_str(), rendered.c_str());

  scope.release();
}



/* ****************************************************************************
*
* check - should Scope::check always return "OK"?
*/
TEST(Scope, check)
{
  Scope        scope;
  Scope        scope1("",     "value");
  Scope        scope2("type", "");
  Scope        scope3("type", "value");
  std::string  checked;
  std::string  expected  = "Empty type in restriction scope";
  std::string  expected1 = "Empty type in restriction scope";
  std::string  expected2 = "Empty value in restriction scope";
  std::string  expected3 = "OK";
  
  checked = scope.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(checked.c_str(), expected.c_str());

  checked = scope1.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(checked.c_str(), expected1.c_str());

  checked = scope2.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(checked.c_str(), expected2.c_str());

  checked = scope3.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(checked.c_str(), expected3.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Scope, present)
{
  Scope   scope("Type", "Value");

  scope.present("", -1);
  scope.present("", 0);
}
