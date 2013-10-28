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
#include "ngsi/ConditionValueList.h"



/* ****************************************************************************
*
* ok - 
*/
TEST(ConditionValueList, ok)
{
  ConditionValueList cvList;
  std::string        out;
  std::string        expected1 = "<attributeList>\n  <attribute>cv1</attribute>\n</attributeList>\n";
  std::string        expected2 = "\"attributeList\" : {\n  \"attribute\" : \"cv1\"\n}\n";
  std::string        expected3 = "\"attributeList\" : {\n  \"attribute\" : \"cv1\",\n  \"attribute\" : \"cv2\"\n}\n";

  out = cvList.render(XML, "");
  EXPECT_STREQ("", out.c_str());

  cvList.push_back("cv1");
  out = cvList.render(XML, "");
  EXPECT_STREQ(expected1.c_str(), out.c_str());

  out = cvList.render(JSON, "");
  EXPECT_STREQ(expected2.c_str(), out.c_str());

  cvList.push_back("cv2");
  out = cvList.render(JSON, "");
  EXPECT_STREQ(expected3.c_str(), out.c_str());

  out = cvList.check(SubscribeContext, XML, "", "", 0);
  EXPECT_STREQ("OK", out.c_str());

  cvList.push_back("");
  out = cvList.check(SubscribeContext, XML, "", "", 0);
  EXPECT_STREQ("empty attribute name", out.c_str());

  // Just to exercise the code
  cvList.present("");

  cvList.release();
  EXPECT_EQ(0, cvList.size());
}
