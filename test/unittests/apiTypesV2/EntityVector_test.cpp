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
* render -
*
*/
TEST(EntityVector, render)
{
  utInit();

  Entity*       eP = new Entity();
  std::string   rendered;
  EntityVector  eV;

  rendered = eV.toJson(NGSI_V2_NORMALIZED);
  EXPECT_STREQ("[]", rendered.c_str());

  eP->entityId.id   = "E_ID";
  eP->entityId.type = "E_TYPE";
  eV.push_back(eP);

  rendered = eV.toJson(NGSI_V2_NORMALIZED);

  eV.release();

  utExit();
}
