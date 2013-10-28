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

#include "ngsi10/UpdateContextSubscriptionResponse.h"



/* ****************************************************************************
*
* constructors - 
*/
TEST(UpdateContextSubscriptionResponse, constructors)
{
  UpdateContextSubscriptionResponse  ucsr1;
  ErrorCode                          ec(SccBadRequest, "RP", "D");
  UpdateContextSubscriptionResponse  ucsr2(ec);
  std::string                        rendered;
  std::string                        expected1 = "<updateContextSubscriptionResponse>\n  <subscribeResponse>\n    <subscriptionId>No Subscription ID</subscriptionId>\n  </subscribeResponse>\n</updateContextSubscriptionResponse>\n";
  std::string                        expected2 = "<updateContextSubscriptionResponse>\n  <subscribeError>\n    <errorCode>\n      <code>400</code>\n      <reasonPhrase>RP</reasonPhrase>\n      <details>D</details>\n    </errorCode>\n  </subscribeError>\n</updateContextSubscriptionResponse>\n";
  
  rendered = ucsr1.render(UpdateContextSubscription, XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  rendered = ucsr2.render(UpdateContextSubscription, XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());
}
