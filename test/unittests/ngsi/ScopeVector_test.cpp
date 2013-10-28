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

#include "ngsi/Scope.h"
#include "ngsi/ScopeVector.h"



/* ****************************************************************************
*
* renderAndRelease - 
*/
TEST(ScopeVector, renderAndRelease)
{
  Scope*         s = new Scope("Type", "Value");
  ScopeVector    sV;
  std::string    expected1 = "";
  std::string    expected2 = "<scope>\n  <operationScope>\n    <type>Type</type>\n    <value>Value</value>\n  </operationScope>\n</scope>\n";
  std::string    rendered;

  rendered = sV.render(XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  sV.push_back(s);

  rendered = sV.render(XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  EXPECT_EQ(sV.size(), 1);
  sV.release();
  EXPECT_EQ(sV.size(), 0);
}



/* ****************************************************************************
*
* check - 
*/
TEST(ScopeVector, check)
{
  Scope*         s1 = new Scope("Type", "Value");
  Scope*         s2 = new Scope("", "Value");
  ScopeVector    sV;
  std::string    expected1 = "OK";
  std::string    expected2 = "Empty type in restriction scope";
  std::string    rendered;
  
  sV.push_back(s1);
  rendered = sV.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  sV.push_back(s2);
  rendered = sV.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());  
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(ScopeVector, present)
{
  ScopeVector   sV;
  Scope         scope("Type", "Value");

  sV.push_back(&scope);
  sV.present("");
}



/* ****************************************************************************
*
* getAndSize - 
*/
TEST(ScopeVector, getAndSize)
{
  ScopeVector   sV;
  Scope         scope0("Type", "Value0");
  Scope         scope1("Type", "Value1");
  Scope         scope2("Type", "Value2");
  Scope*        scopeP;

  sV.push_back(&scope0);
  sV.push_back(&scope1);
  sV.push_back(&scope2);

  EXPECT_EQ(3, sV.size());

  scopeP = sV.get(0);
  EXPECT_STREQ("Value0", scopeP->value.c_str());

  scopeP = sV.get(1);
  EXPECT_STREQ("Value1", scopeP->value.c_str());

  scopeP = sV.get(2);
  EXPECT_STREQ("Value2", scopeP->value.c_str());
}
