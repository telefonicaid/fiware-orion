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
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/Restriction.h"
#include "ngsi/Scope.h"



/* ****************************************************************************
*
* check - should Restriction::check always return "OK"?
*/
TEST(Restriction, check)
{
  Restriction  restriction;
  std::string  checked;
  std::string  expected0 = "OK";
  std::string  expected1 = "empty restriction";
  std::string  expected2 = "Empty type in restriction scope";
  std::string  expected3 = "OK";
  Scope*       scopeP    = new Scope("", "Value");

  checked = restriction.check(0);
  EXPECT_EQ(expected0, checked);

  checked = restriction.check(1);
  EXPECT_EQ(expected1, checked);

  restriction.scopeVector.push_back(scopeP);
  checked = restriction.check(1);
  EXPECT_EQ(expected2, checked);

  scopeP->type = "Type";
  checked = restriction.check(1);
  EXPECT_EQ(expected3, checked);
}



/* ****************************************************************************
*
* render - 
*/
TEST(Restriction, render)
{
  Restriction  restriction;
  std::string  rendered;
  std::string  expected = "";

  rendered = restriction.render(0, false);
  EXPECT_STREQ(expected.c_str(), rendered.c_str());
}
