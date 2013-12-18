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

#include "ngsi/SubscriptionId.h"



/* ****************************************************************************
*
* constructors - 
*/
TEST(SubscriptionId, constructors)
{
  SubscriptionId s1;
  SubscriptionId s2("subId");

  EXPECT_EQ("", s1.string);
  EXPECT_EQ("subId", s2.string);
}



/* ****************************************************************************
*
* check - 
*/
TEST(SubscriptionId, check)
{
  SubscriptionId  sId;
  std::string     checked;
  std::string     expected1 = "bad length (24 chars expected)";
  std::string     expected2 = "invalid char in ID string";
  std::string     expected3 = "OK";

  sId.set("SUB_123");
  checked = sId.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), checked.c_str());

  sId.set("SUB_12345678901234567890");
  checked = sId.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  sId.set("012345678901234567890123");
  checked = sId.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), checked.c_str());
}



/* ****************************************************************************
*
* setGetAndIsEmpty - 
*/
TEST(SubscriptionId, setGetAndIsEmpty)
{
  SubscriptionId  sId;
  std::string     out;
  std::string     expected = "OK";

  sId.set("SUB_123");
  out = sId.get();
  EXPECT_STREQ("SUB_123", out.c_str());

  EXPECT_FALSE(sId.isEmpty());
  sId.set("");
  EXPECT_TRUE(sId.isEmpty());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(SubscriptionId, present)
{
  SubscriptionId  sId;

  sId.set("SUB_123");
  sId.present("");

  sId.set("");
  sId.present("");
}



/* ****************************************************************************
*
* render
*/
TEST(SubscriptionId, render)
{
  SubscriptionId  sId;
  std::string     rendered;
  std::string     expected1 = "<subscriptionId>000000000000000000000000</subscriptionId>\n";
  std::string     expected2 = "<subscriptionId>012345012345012345012345</subscriptionId>\n";
  std::string     expected3 = "\"subscriptionId\" : \"012345012345012345012345\"\n";

  sId.set("");
  rendered = sId.render(UnsubscribeContext, XML, ""); // subscriptionId is MANDATORY for RegisterContext 
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  sId.set("012345012345012345012345");
  rendered = sId.render(UnsubscribeContext, XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());
  
  rendered = sId.render(UnsubscribeContext, JSON, "");
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());

  sId.release(); // just to exercise the code
}
