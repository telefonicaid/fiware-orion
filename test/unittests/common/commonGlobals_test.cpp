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

#include "common/globals.h"

#include "unittest.h"



/* ****************************************************************************
*
* isTrue - 
*/
TEST(commonGlobals, isTrue)
{
   bool bTrue;

   bTrue = isTrue("true");
   EXPECT_TRUE(bTrue);
   bTrue = isTrue("1");
   EXPECT_TRUE(bTrue);

   bTrue = isTrue("TRUE");
   EXPECT_FALSE(bTrue);
   bTrue = isTrue("True");
   EXPECT_FALSE(bTrue);
   bTrue = isTrue("YES");
   EXPECT_FALSE(bTrue);
   bTrue = isTrue("Yes");
   EXPECT_FALSE(bTrue);
   bTrue = isTrue("yes");
   EXPECT_FALSE(bTrue);
   bTrue = isTrue("0");
   EXPECT_FALSE(bTrue);
}



/* ****************************************************************************
*
* isFalse - 
*/
TEST(commonGlobals, isFalse)
{
   bool bFalse;

   bFalse = isFalse("false");
   EXPECT_TRUE(bFalse);
   bFalse = isFalse("0");
   EXPECT_TRUE(bFalse);

   bFalse = isFalse("FALSE");
   EXPECT_FALSE(bFalse);
   bFalse = isFalse("False");
   EXPECT_FALSE(bFalse);
   bFalse = isFalse("NO");
   EXPECT_FALSE(bFalse);
   bFalse = isFalse("No");
   EXPECT_FALSE(bFalse);
   bFalse = isFalse("no");
   EXPECT_FALSE(bFalse);
   bFalse = isFalse("1");
   EXPECT_FALSE(bFalse);
}