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

#include "ngsi/ContextRegistrationResponseVector.h"



/* ****************************************************************************
*
* all - 
*/
TEST(ContextRegistrationResponseVector, all)
{
  ContextRegistrationResponse        crr;
  ContextRegistrationResponseVector  crrV;
  std::string                        rendered;

  crr.contextRegistration.providingApplication.set("10.1.1.1://nada");

  // Empty vector gives empty rendered result
  rendered = crrV.render(XML, "");
  EXPECT_STREQ("", rendered.c_str());

  crrV.push_back(&crr);

  // presenting - just to exercise the code
  crrV.present("");

  // check OK
  rendered = crrV.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ("OK", rendered.c_str());

  EntityId             eId;   // Empty ID
  std::string          expected = "empty entityId:id";

  crr.contextRegistration.entityIdVector.push_back(&eId);
  rendered = crrV.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected.c_str(), rendered.c_str());
}
