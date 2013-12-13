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
#include "ngsi/AttributeExpression.h"



/* ****************************************************************************
*
* ok - 
*/
TEST(AttributeExpression, ok)
{
   AttributeExpression ae;

   ae.set("AE");
   EXPECT_STREQ("AE", ae.get().c_str());

   ae.set("");
   EXPECT_STREQ("", ae.render(XML, "", false).c_str());

   ae.set("AE");
   EXPECT_STREQ("<attributeExpression>AE</attributeExpression>\n", ae.render(XML, "", false).c_str());
   EXPECT_STREQ("\"attributeExpression\" : \"AE\"\n", ae.render(JSON, "", false).c_str());

   EXPECT_STREQ("AE", ae.c_str());
}
