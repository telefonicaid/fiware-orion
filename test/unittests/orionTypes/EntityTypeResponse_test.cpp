/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*/
#include "orionTypes/EntityTypeResponse.h"
#include "unittests/unittest.h"



/* ****************************************************************************
*
* check
*/
TEST(EntityTypeResponse, check)
{
  utInit();

  EntityTypeResponse etR1;
  EntityTypeResponse etR2;

  etR1.entityType.type = "myType";
  etR2.entityType.type = "";

  EXPECT_EQ("OK", etR1.check(V1, false, false, false, ""));
  EXPECT_NE("OK", etR2.check(V1, false, false, false, ""));
  EXPECT_NE("OK", etR1.check(V1, false, false, false, "foo"));

  utExit();
}
