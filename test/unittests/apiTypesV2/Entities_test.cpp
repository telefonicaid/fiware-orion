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
#include "apiTypesV2/Entities.h"
#include "unittests/unittest.h"



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Entities, present)
{
  utInit();

  Entity* enP    = new Entity();
  enP->id        = "E";
  enP->type      = "T";
  enP->isPattern = "false";

  ContextAttribute* caP = new ContextAttribute("A", "T", "val");
  enP->attributeVector.push_back(caP);

  Entities ens;
  ens.vec.push_back(enP);
  ens.oe.fill(SccNone, "Lorem ipsum", "FooError");

  ens.present("");

  utExit();
}



/* ****************************************************************************
*
* check
*/
TEST(Entities, check)
{
  utInit();

  Entity* enP;

  // Entities with ok Entity inside
  enP            = new Entity();
  enP->id        = "E";
  enP->type      = "T";
  enP->isPattern = "false";
  Entities ens1;
  ens1.vec.push_back(enP);

  // Entities with nok Entity inside
  enP            = new Entity();
  enP->id        = "";
  enP->type      = "T";
  enP->isPattern = "false";
  Entities ens2;
  ens2.vec.push_back(enP);

  EXPECT_EQ("OK", ens1.check(EntitiesRequest));
  EXPECT_EQ("entity id length: 0, min length supported: 1", ens2.check(EntitiesRequest));

  utExit();
}
