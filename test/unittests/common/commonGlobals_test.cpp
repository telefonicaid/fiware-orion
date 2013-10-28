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
#include "common/globals.h"


/* ****************************************************************************
*
* isTrue - 
*/
TEST(commonGlobals, isTrue)
{
   bool bTrue;

   bTrue = isTrue("TRUE");
   EXPECT_TRUE(bTrue) << "'TRUE' should be true";
   bTrue = isTrue("True");
   EXPECT_TRUE(bTrue) << "'True' should be true";
   bTrue = isTrue("true");
   EXPECT_TRUE(bTrue) << "'true' should be true";

   bTrue = isTrue("YES");
   EXPECT_TRUE(bTrue) << "'YES' should be true";
   bTrue = isTrue("Yes");
   EXPECT_TRUE(bTrue) << "'Yes' should be true";
   bTrue = isTrue("yes");
   EXPECT_TRUE(bTrue) << "'yes' should be true";
}



/* ****************************************************************************
*
* isFalse - 
*/
TEST(commonGlobals, isFalse)
{
   bool bFalse;

   bFalse = isFalse("FALSE");
   EXPECT_TRUE(bFalse) << "'FALSE' should be false";
   bFalse = isFalse("False");
   EXPECT_TRUE(bFalse) << "'False' should be false";
   bFalse = isFalse("false");
   EXPECT_TRUE(bFalse) << "'false' should be false";

   bFalse = isFalse("NO");
   EXPECT_TRUE(bFalse) << "'NO' should be false";
   bFalse = isFalse("No");
   EXPECT_TRUE(bFalse) << "'No' should be false";
   bFalse = isFalse("no");
   EXPECT_TRUE(bFalse) << "'no' should be false";
}



/* ****************************************************************************
*
* parse8601 - 
*/
TEST(commonGlobals, parse8601)
{
   int       secs;
   const int oneYear    = 365 * 24 * 3600 * 1;
   const int oneMonth   =  30 * 24 * 3600 * 1;
   const int oneWeek    =   7 * 24 * 3600 * 1;
   const int oneDay     =       24 * 3600 * 1;
   const int oneHour    =            3600 * 1;
   const int oneMinute  =              60 * 1;
   const int oneSecond  =                   1;
   const int twoYears   = 365 * 24 * 3600 * 2;
   const int twoMonths  =  30 * 24 * 3600 * 2;
   const int twoWeeks   =         oneWeek * 2;
   const int twoDays    =       24 * 3600 * 2;
   const int twoHours   =            3600 * 2;
   const int twoMinutes =              60 * 2;
   const int twoSeconds =       oneSecond * 2;
   const int threeYearsOneMonthOneDayOneHourOneMinuteAndElevenSeconds = 3 * oneYear + oneMonth + oneDay + oneHour + oneMinute + 11;

   secs = parse8601("P2Y");
   EXPECT_EQ(twoYears, secs) << "bad value for two years";
   secs = parse8601("PT2Y");
   EXPECT_EQ(-1, secs) << "PT2Y should return -1 - parse error ...";

   secs = parse8601("P2M");
   EXPECT_EQ(twoMonths, secs) << "bad value for two months";

   secs = parse8601("P2W");
   EXPECT_EQ(twoWeeks, secs) << "bad value for two weeks";
   secs = parse8601("PT2W");
   EXPECT_EQ(-1, secs) << "PT2W should return -1 - parse error ...";

   secs = parse8601("P2D");
   EXPECT_EQ(twoDays, secs) << "bad value for two days";
   secs = parse8601("PT2D");
   EXPECT_EQ(-1, secs) << "PT2D should return -1 - parse error ...";

   secs = parse8601("PT2H");
   EXPECT_EQ(twoHours, secs) << "bad value for two hours";
   secs = parse8601("P2H");
   EXPECT_EQ(-1, secs) << "P2H should return -1 - parse error ...";

   secs = parse8601("PT2M");
   EXPECT_EQ(twoMinutes, secs) << "bad value for two minutes";

   secs = parse8601("PT2S");
   EXPECT_EQ(twoSeconds, secs) << "bad value for two seconds";
   secs = parse8601("P2S");
   EXPECT_EQ(-1, secs) << "P2S should return -1 - parse error ...";

   secs = parse8601("P3Y1M1DT1H1M11S");
   EXPECT_EQ(threeYearsOneMonthOneDayOneHourOneMinuteAndElevenSeconds, secs) << "parse error for 'P3Y1M1DT1H1M11S'";
}
