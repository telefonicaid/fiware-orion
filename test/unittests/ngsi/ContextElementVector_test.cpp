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

#include "ngsi/ContextElementVector.h"

#include "unittest.h"



/* ****************************************************************************
*
* render - 
*
*/
TEST(ContextElementVector, render)
{
  ContextElement*       ceP = new ContextElement();
  EntityId              eId("E_ID", "E_TYPE");
  std::string           rendered;
  ContextElementVector  ceV;

  rendered = ceV.render(V1, false, UpdateContextElement, false);
  EXPECT_STREQ("", rendered.c_str());

  ceP->entityId = eId;
  ceV.push_back(ceP);

  rendered = ceV.render(V1, false, UpdateContextElement, false);

  ceV.release();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code ...
*/
TEST(ContextElementVector, present)
{
  ContextElement        ce;
  EntityId              eId("E_ID", "E_TYPE");
  ContextElementVector  ceV;

  ce.entityId = eId;
  ceV.push_back(&ce);

  ceV.present("  ");
}
