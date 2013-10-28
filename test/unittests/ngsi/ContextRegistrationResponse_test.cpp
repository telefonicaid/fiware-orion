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

#include "ngsi/ContextRegistrationResponse.h"



/* ****************************************************************************
*
* render - 
*/
TEST(ContextRegistrationResponse, render)
{
  ContextRegistrationResponse  crr;
  std::string                  rendered;
  std::string                  expected1 = "<contextRegistrationResponse>\n  <contextRegistration>\n  </contextRegistration>\n</contextRegistrationResponse>\n";
  std::string                  expected2 = "<contextRegistrationResponse>\n  <contextRegistration>\n  </contextRegistration>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase></reasonPhrase>\n  </errorCode>\n</contextRegistrationResponse>\n";

  crr.errorCode.code = NO_ERROR_CODE;
  rendered = crr.render(XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  crr.errorCode.code = 400;
  rendered = crr.render(XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* check - 
*/
TEST(ContextRegistrationResponse, check)
{
  ContextRegistrationResponse  crr;
  std::string                  checked;
  std::string                  expected = "no providing application";

  checked = crr.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected.c_str(), checked.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(ContextRegistrationResponse, present)
{
  ContextRegistrationResponse  crr;

  crr.present("");
}
