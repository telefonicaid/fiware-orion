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

#include "ngsi9/SubscribeContextAvailabilityResponse.h"


/* ****************************************************************************
*
* constructors - 
*/
TEST(SubscribeContextAvailabilityResponse, constructors)
{
  SubscribeContextAvailabilityResponse* scar1 = new SubscribeContextAvailabilityResponse();
  SubscribeContextAvailabilityResponse  scar2("012345678901234567890123", "PT1S");
  ErrorCode                             ec(SccBadRequest, "Reason", "Detail");
  SubscribeContextAvailabilityResponse  scar3("012345678901234567890124", ec);

  EXPECT_EQ("", scar1->subscriptionId.get());
  delete(scar1);

  EXPECT_EQ("012345678901234567890123", scar2.subscriptionId.get());

  EXPECT_EQ("012345678901234567890124", scar3.subscriptionId.get());
  EXPECT_EQ(SccBadRequest, scar3.errorCode.code);
}
