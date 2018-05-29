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

#include "ngsi/Duration.h"



/* ****************************************************************************
*
* check - 
*/
TEST(Duration, check)
{
  Duration     d;
  std::string  out;

  out = d.check();
  EXPECT_STREQ("OK", out.c_str());

  d.set("PT1S");
  out = d.check();
  EXPECT_STREQ("OK", out.c_str());

  d.set("PT1A");
  out = d.check();
  EXPECT_STREQ("syntax error in duration string", out.c_str());
}



/* ****************************************************************************
*
* present - 
*/
TEST(Duration, present)
{
  Duration     d;

  d.present("");

  d.set("PT1S");
  d.present("");
}



/* ****************************************************************************
*
* isEmpty - 
*/
TEST(Duration, isEmpty)
{
   Duration d;

   d.set("");
   EXPECT_EQ(true, d.isEmpty());
}
