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
#include <string>

#include "apiTypesV2/EntityVector.h"
#include "unittests/unittest.h"



/* ****************************************************************************
*
* check
*/
TEST(EntityVector, check)
{
  utInit();

  Entity* enP;

  // EntityVector with ok Entity inside
  enP            = new Entity();
  enP->id        = "E";
  enP->type      = "T";
  enP->isPattern = "false";
  EntityVector enV1;
  enV1.push_back(enP);

  // EntityVector with nok Entity inside
  enP            = new Entity();
  enP->id        = "";
  enP->type      = "T";
  enP->isPattern = "false";
  EntityVector enV2;
  enV2.push_back(enP);

  EXPECT_EQ("OK", enV1.check(V2, EntitiesRequest));
  EXPECT_EQ("entity id length: 0, min length supported: 1", enV2.check(V2, EntitiesRequest));

  utExit();
}


/* ****************************************************************************
*
* render -
*
*/
TEST(EntityVector, render)
{
  Entity*       eP = new Entity();
  std::string   rendered;
  EntityVector  eV;

  rendered = eV.toJsonV1(false, UpdateContextElement, false);
  EXPECT_STREQ("", rendered.c_str());

  eP->id   = "E_ID";
  eP->type = "E_TYPE";
  eV.push_back(eP);

  rendered = eV.toJsonV1(false, UpdateContextElement, false);

  eV.release();
}
