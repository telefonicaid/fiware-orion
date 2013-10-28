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

#include "convenienceMap/mapPostEntityByIdAttributeByName.h"
#include "convenienceMap/mapGetEntityByIdAttributeByName.h"
#include "ngsi9/RegisterContextResponse.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"
#include "convenience/RegisterProviderRequest.h"

#include "testInit.h"



/* ****************************************************************************
*
* mapPostEntityByIdAttributeByName - 
*/
TEST(mapPostEntityByIdAttributeByName, ok)
{
  RegisterProviderRequest              rpr;
  RegisterContextResponse              response;
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityResponse  discoveryResponse;

  /* Set timer */
  Timer* t = new Timer();
  setTimer(t);

  /* Set up the database */
  setupDatabase();

  rpr.duration.set("PT1H");
  ms = mapPostEntityByIdAttributeByName("Entity01", "Attr01", &rpr, &response);
  EXPECT_EQ(SccOk, ms);

  ms = mapGetEntityByIdAttributeByName("Entity01", "Attr01", &discoveryResponse);
  EXPECT_EQ(SccOk, ms);
  ASSERT_EQ(1, discoveryResponse.responseVector.size());
  ASSERT_EQ(1, discoveryResponse.responseVector.get(0)->contextRegistration.entityIdVector.size());
  EXPECT_STREQ("Entity01", discoveryResponse.responseVector.get(0)->contextRegistration.entityIdVector.get(0)->id.c_str());

  /* Delete timer */
  delete t;
}
