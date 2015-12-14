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

   secs = parse8601("P0YT12.005S");
   EXPECT_EQ(12, secs) << "parse error for 'P0YT12.005S'";

   secs = parse8601("P0YT12.49999S");
   EXPECT_EQ(12, secs) << "round error for 'P0YT12.49999S'";

   secs = parse8601("P0YT12.50S");
   EXPECT_EQ(13, secs) << "round error for 'P0YT12.50S'";

   secs = parse8601("P0YT12.51S");
   EXPECT_EQ(13, secs) << "round error for 'P0YT12.51S'";


   //
   // Errors
   //
   secs = parse8601("");
   EXPECT_EQ(-1, secs);

   secs = parse8601("Q1");
   EXPECT_EQ(-1, secs);

   secs = parse8601("P");
   EXPECT_EQ(-1, secs);

   secs = parse8601("Px");
   EXPECT_EQ(-1, secs);

   secs = parse8601("P4");
   EXPECT_EQ(-1, secs);

   secs = parse8601("P4Y1");
   EXPECT_EQ(-1, secs);

   secs = parse8601("PT4Y");
   EXPECT_EQ(-1, secs);

   secs = parse8601("PT99Y");
   EXPECT_EQ(-1, secs);

   secs = parse8601("PY99");
   EXPECT_EQ(-1, secs);
}



/* ****************************************************************************
*
* toSeconds - 
*/
TEST(commonGlobals, toSeconds)
{
  int secs;
  long long longsecs;

  // 3 years
  secs = toSeconds(3, 'Y', true);
  EXPECT_EQ(3 * 365 * 24 * 3600, secs);

  // error
  secs = toSeconds(3, 'Y', false);
  EXPECT_EQ(-1, secs);

  // 3 months
  secs = toSeconds(3, 'M', true);
  EXPECT_EQ(3 * 30 * 24 * 3600, secs);

  // 3 weeks
  secs = toSeconds(3, 'W', true);
  EXPECT_EQ(3 * 7 * 24 * 3600, secs);

  // 3 days
  secs = toSeconds(3, 'D', true);
  EXPECT_EQ(3 * 24 * 3600, secs);

  // 3 hours
  secs = toSeconds(3, 'H', false);
  EXPECT_EQ(3 * 3600, secs);

  // 3 minutes
  secs = toSeconds(3, 'M', false);
  EXPECT_EQ(3 * 60, secs);

  // 3 seconds
  secs = toSeconds(3, 'S', false);
  EXPECT_EQ(3, secs);

  // error
  secs = toSeconds(3, 'f', false);
  EXPECT_EQ(-1, secs);

  longsecs = toSeconds(30, 'Y', true);
  EXPECT_EQ(946080000, longsecs);

  longsecs = toSeconds(300, 'Y', true);
  EXPECT_EQ(9460800000L, longsecs);
}


/* ****************************************************************************
*
* getCurrentTime - 
*/
TEST(commonGlobals, getCurrentTime)
{
  int now;

  // 1. No timer
  setTimer(NULL);
  now = getCurrentTime();
  EXPECT_EQ(-1, now);

  utInit();  // timer is set up inside utInit
  now = getCurrentTime();
  EXPECT_TRUE(now != -1);
  utExit();
}
