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

#include "convenienceMap/mapPostContextEntitiesByEntityId.h"
#include "convenienceMap/mapGetContextEntitiesByEntityId.h"
#include "ngsi9/RegisterContextResponse.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"
#include "convenience/RegisterProviderRequest.h"

#include "testInit.h"



/* ****************************************************************************
*
* mapContextEntitiesByEntityId - 
*/
TEST(mapContextEntitiesByEntityId, ok)
{
  HttpStatusCode                       ms;
  RegisterProviderRequest              rpr;
  RegisterContextResponse              rcr;
  DiscoverContextAvailabilityResponse  dcar;

  /* Set timer */
  Timer* t = new Timer();
  setTimer(t);

  /* Set up the database */
  setupDatabase();

  rpr.duration.set("PT1S");
  rpr.providingApplication.set("http://localhost:99/abc");

  ms = mapPostContextEntitiesByEntityId("Entity01", &rpr, &rcr);
  EXPECT_EQ(SccOk, ms);
  EXPECT_STREQ("PT1S", rcr.duration.string.c_str());

  ms = mapGetContextEntitiesByEntityId("Entity01", &dcar);
  EXPECT_EQ(SccOk, ms);
  ASSERT_EQ(1, dcar.responseVector.size());

  /* Delete timer */
  delete t;
}
