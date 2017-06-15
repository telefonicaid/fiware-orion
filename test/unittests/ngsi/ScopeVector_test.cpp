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
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "ngsi/Scope.h"
#include "ngsi/ScopeVector.h"

#include "unittest.h"



/* ****************************************************************************
*
* renderAndRelease -
*
*/
TEST(ScopeVector, renderAndRelease)
{
  Scope*         s = new Scope("Type", "Value");
  ScopeVector    sV;

  utInit();

  {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    sV.toJson(writer);
    EXPECT_STREQ("", sb.GetString());
  }

  sV.push_back(s);

  {
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    writer.SetIndent(' ', 2);
    sV.toJson(writer);
  }

  EXPECT_EQ(sV.size(), 1);
  sV.release();
  EXPECT_EQ(sV.size(), 0);

  utExit();
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

  utInit();

  sV.push_back(s1);
  rendered = sV.check(RegisterContext, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  sV.push_back(s2);
  rendered = sV.check(RegisterContext, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(ScopeVector, present)
{
  ScopeVector   sV;
  Scope         scope("Type", "Value");

  utInit();

  sV.push_back(&scope);
  sV.present("");

  utExit();
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

  utInit();

  sV.push_back(&scope0);
  sV.push_back(&scope1);
  sV.push_back(&scope2);

  EXPECT_EQ(3, sV.size());

  scopeP = sV[0];
  EXPECT_STREQ("Value0", scopeP->value.c_str());

  scopeP = sV[1];
  EXPECT_STREQ("Value1", scopeP->value.c_str());

  scopeP = sV[2];
  EXPECT_STREQ("Value2", scopeP->value.c_str());

  utExit();
}
