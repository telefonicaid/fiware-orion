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
#include "convenience/UpdateContextElementRequest.h"
#include "convenience/ContextAttributeResponseVector.h"



/* ****************************************************************************
*
* render - 
*/
TEST(UpdateContextElementRequest, render)
{
  UpdateContextElementRequest     ucer;
  ContextAttribute                ca("caName", "caType", "caValue");
  std::string                     out;
  std::string                     expected = "<updateContextElementRequest>\n  <attributeDomainName>ADN</attributeDomainName>\n  <contextAttributeList>\n    <contextAttribute>\n      <name>caName</name>\n      <type>caType</type>\n      <contextValue>caValue</contextValue>\n    </contextAttribute>\n  </contextAttributeList>\n</updateContextElementRequest>\n";

  // Just the normal case
  ucer.attributeDomainName.set("ADN");
  ucer.contextAttributeVector.push_back(&ca);

  out = ucer.render(XML, "");
  EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* check - 
*/
TEST(UpdateContextElementRequest, check)
{
  UpdateContextElementRequest     ucer;
  ContextAttribute                ca("caName", "caType", "caValue");
  std::string                     out;
  std::string                     expected1 = "<updateContextElementResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>PRE Error</reasonPhrase>\n  </errorCode>\n</updateContextElementResponse>\n";
  std::string                     expected2 = "OK";
  std::string                     expected3 = "OK";
  std::string                     expected4 = "<updateContextElementResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing attribute name</reasonPhrase>\n  </errorCode>\n</updateContextElementResponse>\n";

  ucer.attributeDomainName.set("ADN");

  // 1. predetectedError
  ucer.contextAttributeVector.push_back(&ca);
  out = ucer.check(UpdateContextElement, XML, "", "PRE Error", 0);
  EXPECT_STREQ(expected1.c_str(), out.c_str());

  // 2. ok
  out = ucer.check(UpdateContextElement, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), out.c_str());

  // 3. bad attributeDomainName
  ucer.attributeDomainName.set("");
  EXPECT_STREQ(expected3.c_str(), out.c_str());

  // 4. bad contextAttributeVector
  ContextAttribute                ca2("", "caType", "caValue");
  ucer.contextAttributeVector.push_back(&ca2);
  out = ucer.check(UpdateContextElement, XML, "", "", 0);
  EXPECT_STREQ(expected4.c_str(), out.c_str());
}



/* ****************************************************************************
*
* present - just exercise the code
*/
TEST(UpdateContextElementRequest, present)
{
  UpdateContextElementRequest     ucer;
  ContextAttribute                ca("caName", "caType", "caValue");

  ucer.attributeDomainName.set("ADN");
  ucer.contextAttributeVector.push_back(&ca);

  ucer.present("");
}



/* ****************************************************************************
*
* release - 
*/
TEST(UpdateContextElementRequest, release)
{
  UpdateContextElementRequest     ucer;
  ContextAttribute*               caP = new ContextAttribute("caName", "caType", "caValue");

  ucer.attributeDomainName.set("ADN");
  ucer.contextAttributeVector.push_back(caP);
  
  ASSERT_EQ(1, ucer.contextAttributeVector.size());

  ucer.release();
  EXPECT_EQ(0, ucer.contextAttributeVector.size());
}
