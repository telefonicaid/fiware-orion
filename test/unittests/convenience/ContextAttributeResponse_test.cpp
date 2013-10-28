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

#include "common/Format.h"
#include "convenience/ContextAttributeResponse.h"



/* ****************************************************************************
*
* render - 
*/
TEST(ContextAttributeResponse, render)
{
  ContextAttribute          ca("caName", "caType", "caValue");
  ContextAttributeResponse  car;
  std::string               out;

  car.contextAttributeVector.push_back(&ca);
  car.statusCode.fill(SccOk, "OK"); 

  out = car.render(XML, "");
}



/* ****************************************************************************
*
* check - 
*/
TEST(ContextAttributeResponse, check)
{
  ContextAttribute          ca("caName", "caType", "caValue");
  ContextAttributeResponse  car;
  std::string               out;
  std::string               expected1 = "OK";
  std::string               expected2 = "<contextAttributeResponse>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>PRE Error</reasonPhrase>\n  </statusCode>\n</contextAttributeResponse>\n";
  std::string               expected3 = "<contextAttributeResponse>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>missing attribute name</reasonPhrase>\n  </statusCode>\n</contextAttributeResponse>\n";

  // 1. OK
  car.contextAttributeVector.push_back(&ca);
  car.statusCode.fill(SccOk, "OK"); 

  out = car.check(UpdateContextAttribute, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), out.c_str());


  // 2. predetectedError
  out = car.check(UpdateContextAttribute, XML, "", "PRE Error", 0);
  EXPECT_STREQ(expected2.c_str(), out.c_str());


  // 3. Bad ContextAttribute
  ContextAttribute          ca2("", "caType", "caValue");
  car.contextAttributeVector.push_back(&ca2);
  
  out = car.check(UpdateContextAttribute, XML, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), out.c_str());
}



/* ****************************************************************************
*
* present - just exercise the code
*/
TEST(ContextAttributeResponse, present)
{
  ContextAttribute          ca("caName", "caType", "caValue");
  ContextAttributeResponse  car;

  car.contextAttributeVector.push_back(&ca);
  car.statusCode.fill(SccOk, "OK");

  car.present("");
}



/* ****************************************************************************
*
* release - just exercise the code
*/
TEST(ContextAttributeResponse, release)
{
  ContextAttribute*         caP = new ContextAttribute("caName", "caType", "caValue");
  ContextAttributeResponse  car;

  car.contextAttributeVector.push_back(caP);
  car.statusCode.fill(SccOk, "OK");

  car.release();
}
