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

#include "ngsi10/UpdateContextResponse.h"



/* ****************************************************************************
*
* constructors - 
*/
TEST(UpdateContextResponse, constructors)
{
  UpdateContextResponse  ucr1;
  ErrorCode              ec(SccBadRequest, "RP", "D");
  UpdateContextResponse  ucr2(ec);
  std::string            rendered;
  std::string            expected1 = "<updateContextResponse>\n</updateContextResponse>\n";
  std::string            expected2 = "<updateContextResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>RP</reasonPhrase>\n    <details>D</details>\n  </errorCode>\n</updateContextResponse>\n";

  rendered = ucr1.render(UpdateContext, XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  rendered = ucr2.render(UpdateContext, XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());
}

