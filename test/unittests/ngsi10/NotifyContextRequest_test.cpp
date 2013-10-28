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

#include "ngsi10/NotifyContextRequest.h"



/* ****************************************************************************
*
* ok - as NotifyContext requests are not received, the struct is made binarily
*/
TEST(NotifyContextRequest, ok)
{
  NotifyContextRequest     ncr;
  ContextElementResponse*  cerP = new ContextElementResponse();
  std::string              rendered;
  std::string              expected = "<notifyContextRequest>\n  <subscriptionId>SUB_1</subscriptionId>\n  <originator>originator</originator>\n  <contextResponseList>\n    <contextElementResponse>\n      <contextElement>\n        <entityId type=\"\" isPattern=\"\">\n          <id></id>\n        </entityId>\n      </contextElement>\n      <statusCode>\n        <code>0</code>\n        <reasonPhrase></reasonPhrase>\n      </statusCode>\n    </contextElementResponse>\n  </contextResponseList>\n</notifyContextRequest>\n";

  ncr.subscriptionId.set("SUB_1");
  ncr.originator.set("originator");
  ncr.contextElementResponseVector.push_back(cerP);

  rendered = ncr.render(NotifyContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

  ncr.present("");
  ncr.release();
}
