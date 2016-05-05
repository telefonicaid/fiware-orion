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

#include "orionTypes/EntityTypeVector.h"
#include "unittest.h"

/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(EntityTypeVector, present)
{
  utInit();

  EntityType et("myType");
  EntityTypeVector etV;
  etV.vec.push_back(&et);

  etV.present("");

  utExit();
}

/* ****************************************************************************
*
* check
*/
TEST(EntityTypeVector, check)
{
  ConnectionInfo ci;

  utInit();

  ci.outMimeType = JSON;

  EntityType et1("myType");
  EntityType et2("");

  // EntityTypeVector with a EntityType that will not fail
  EntityTypeVector etV1;
  etV1.push_back(&et1);

  // EntityTypeVector with a EntityType that will fail
  EntityTypeVector etV2;
  etV2.push_back(&et2);

  EXPECT_EQ("OK", etV1.check(&ci, "", ""));

  EXPECT_EQ("foo", etV1.check(&ci, "", "foo"));

  EXPECT_EQ("Empty Type", etV2.check(&ci, "", ""));

  utExit();
}
