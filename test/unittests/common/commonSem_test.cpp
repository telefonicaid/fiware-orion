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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/sem.h"



/* ****************************************************************************
*
* semToString - 
*
* The real test here would be to create a few threads and play with the semaphores ... 
*
* Here I only exercise the code (which is also valid and important)
*
* This is only a wrapper function for the real semaphore function and as we're only testing
* the wrapper functions, there is no reason to do any more ...
*/
TEST(commonSem, unique)
{
   int s;

   semInit();

   s = reqSemGive(__FUNCTION__, "test");
   EXPECT_EQ(0, s);

   bool taken;
   s = reqSemTake(__FUNCTION__, "test", SemReadWriteOp, &taken);
   EXPECT_EQ(0, s);
   EXPECT_TRUE(taken);
}
