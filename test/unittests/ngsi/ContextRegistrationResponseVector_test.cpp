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
  rendered = crrV.render(false);
  EXPECT_EQ("", rendered);

  crrV.push_back(&crr);

  // check OK
  rendered = crrV.check(V1, RegisterContext, "", 0);
  EXPECT_EQ("OK", rendered);

  // Now telling the crr that we've found an instance of '<entityIdList></entityIdList>
  // but without any entities inside the vector
  crr.contextRegistration.entityIdVectorPresent = true;
  rendered = crrV.check(V1, RegisterContext, "", 0);
  EXPECT_EQ("Empty entityIdVector", rendered);

  EntityId             eId;   // Empty ID

  crr.contextRegistration.entityIdVector.push_back(&eId);
  rendered = crrV.check(V1, RegisterContext, "", 0);
  EXPECT_EQ("empty entityId:id", rendered);
}
