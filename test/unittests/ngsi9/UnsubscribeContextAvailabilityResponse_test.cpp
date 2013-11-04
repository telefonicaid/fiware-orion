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

#include "ngsi9/UnsubscribeContextAvailabilityResponse.h"
#include "ngsi/StatusCode.h"
#include "ngsi/ErrorCode.h"



/* ****************************************************************************
*
* constructorsAndRender - 
*/
TEST(UnsubscribeContextAvailabilityResponse, constructorsAndRender)
{
  UnsubscribeContextAvailabilityResponse  ucar1;
  SubscriptionId                          subscriptionId;

  subscriptionId.set("111122223333444455556666");

  UnsubscribeContextAvailabilityResponse  ucar2(subscriptionId);
  ErrorCode                               ec(SccBadRequest, "RP", "D");
  UnsubscribeContextAvailabilityResponse  ucar3(ec);
  std::string                             rendered;
  std::string                             expected = "<unsubscribeContextAvailabilityResponse>\n  <subscriptionId>No Subscription ID</subscriptionId>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>RP</reasonPhrase>\n    <details>D</details>\n  </statusCode>\n</unsubscribeContextAvailabilityResponse>\n";
;

  EXPECT_EQ(0,                    ucar1.statusCode.code);
  EXPECT_EQ(subscriptionId.get(), ucar2.subscriptionId.get());
  EXPECT_EQ(SccBadRequest,        ucar3.statusCode.code);

  rendered = ucar3.render(UnsubscribeContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());
}
