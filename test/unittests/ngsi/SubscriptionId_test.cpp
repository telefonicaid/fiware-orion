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

#include "ngsi/SubscriptionId.h"

#include "unittest.h"



/* ****************************************************************************
*
* constructors -
*/
TEST(SubscriptionId, constructors)
{
  SubscriptionId s1;
  SubscriptionId s2("subId");

  utInit();

  EXPECT_EQ("", s1.string);
  EXPECT_EQ("subId", s2.string);

  utExit();
}



/* ****************************************************************************
*
* check -
*/
TEST(SubscriptionId, check)
{
  SubscriptionId  sId;
  std::string     checked;

  utInit();

  sId.set("SUB_123");
  checked = sId.check();
  EXPECT_STREQ("bad length - 24 chars expected", checked.c_str());

  sId.set("SUB_12345678901234567890");
  checked = sId.check();
  EXPECT_STREQ("invalid char in ID string", checked.c_str());

  sId.set("012345678901234567890123");
  checked = sId.check();
  EXPECT_STREQ("OK", checked.c_str());

  utExit();
}



/* ****************************************************************************
*
* setGetAndIsEmpty -
*/
TEST(SubscriptionId, setGetAndIsEmpty)
{
  SubscriptionId  sId;
  std::string     out;

  utInit();

  sId.set("SUB_123");
  out = sId.get();
  EXPECT_STREQ("SUB_123", out.c_str());

  EXPECT_FALSE(sId.isEmpty());
  sId.set("");
  EXPECT_TRUE(sId.isEmpty());

  utExit();
}



/* ****************************************************************************
*
* render
*/
TEST(SubscriptionId, render)
{
  SubscriptionId  sId;
  std::string     out;
  const char*     outfile1 = "ngsi.subscriptionId.render2.middle.json";

  utInit();

  sId.set("012345012345012345012345");

  out = sId.toJsonV1(UnsubscribeContext, false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  sId.release(); // just to exercise the code

  utExit();
}
