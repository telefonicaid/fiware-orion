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
#include "convenience/AppendContextElementResponse.h"



/* ****************************************************************************
*
* render - 
*/
TEST(AppendContextElementResponse, render)
{
  AppendContextElementResponse  acer;
  ContextAttributeResponse      car;
  std::string                   out;
  std::string                   expected1 = "<appendContextAttributeResponse>\n</appendContextAttributeResponse>\n";
  std::string                   expected2 = "<appendContextAttributeResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>bad request</reasonPhrase>\n    <details>very bad request</details>\n  </errorCode>\n</appendContextAttributeResponse>\n";

  // 1. empty acer
  out = acer.render(XML, "");
  EXPECT_EQ(expected1, out);

  // 2. errorCode 'active'
  acer.errorCode.fill(SccBadRequest, "bad request", "very bad request");
  out = acer.render(XML, "");
  EXPECT_EQ(expected2, out);
}   



/* ****************************************************************************
*
* check - 
*/
TEST(AppendContextElementResponse, check)
{
  AppendContextElementResponse  acer;
  ContextAttributeResponse      car;
  ContextAttribute              ca("", "TYPE", "VALUE"); // empty name, thus provoking error
  std::string                   out;
  std::string                   expected1 = "<appendContextAttributeResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>PRE ERR</reasonPhrase>\n  </errorCode>\n</appendContextAttributeResponse>\n";
  std::string                   expected2 = "<appendContextAttributeResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase><contextAttributeResponse>\n  <statusCode>\n    <code>400</code>\n    <reasonPhrase>missing attribute name</reasonPhrase>\n  </statusCode>\n</contextAttributeResponse>\n</reasonPhrase>\n  </errorCode>\n</appendContextAttributeResponse>\n";
  std::string                   expected3 = "OK";

  // 1. predetected error
  out = acer.check(IndividualContextEntity, XML, "", "PRE ERR", 0);
  EXPECT_EQ(expected1, out);

  // 2. bad contextResponseVector
  car.contextAttributeVector.push_back(&ca);
  acer.contextResponseVector.push_back(&car);
  out = acer.check(IndividualContextEntity, XML, "", "", 0);
  EXPECT_EQ(expected2, out);

  // 3. OK
  ca.name = "NAME";
  out = acer.check(IndividualContextEntity, XML, "", "", 0);
  EXPECT_EQ(expected3, out);
}   



/* ****************************************************************************
*
* release - 
*/
TEST(AppendContextElementResponse, release)
{
  AppendContextElementResponse  acer;
  ContextAttributeResponse*     carP = new ContextAttributeResponse();
  ContextAttribute*             caP  = new ContextAttribute("NAME", "TYPE", "VALUE");

  carP->contextAttributeVector.push_back(caP);
  acer.contextResponseVector.push_back(carP);

  EXPECT_EQ(1, carP->contextAttributeVector.size());
  EXPECT_EQ(1, acer.contextResponseVector.size());
  acer.release();
  EXPECT_EQ(0, acer.contextResponseVector.size());
}
