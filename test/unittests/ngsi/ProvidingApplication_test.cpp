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

#include "ngsi/ProvidingApplication.h"



/* ****************************************************************************
*
* render - 
*/
TEST(ProvidingApplication, render)
{
  ProvidingApplication  pa;
  std::string           out;

  out = pa.render(XML, "");
  EXPECT_STREQ("", out.c_str());

  pa.set("PA");
  out = pa.render(XML, "");
  EXPECT_STREQ("<providingApplication>PA</providingApplication>\n", out.c_str());
}



/* ****************************************************************************
*
* present - just to exercise the code ...
*/
TEST(ProvidingApplication, present)
{
  ProvidingApplication  pa;

  pa.present("");

  pa.set("PA");
  pa.present("");
}



/* ****************************************************************************
*
* c_str - 
*/
TEST(ProvidingApplication, c_str)
{
  ProvidingApplication  pa;

  pa.set("PA");
  EXPECT_STREQ("PA", pa.c_str());
}
