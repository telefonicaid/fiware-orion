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

#include "ngsi/Reference.h"



/* ****************************************************************************
*
* check - should Reference::check always return "OK"?
*/
TEST(Reference, check)
{
  Reference    reference;
  std::string  checked;
  std::string  expected = "OK";
  
  checked = reference.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(checked.c_str(), expected.c_str());
}



/* ****************************************************************************
*
* isEmptySetAndGet - 
*/
TEST(Reference, isEmptySetAndGet)
{
  Reference    reference;

  reference.set("REF");
  EXPECT_STREQ("REF", reference.get().c_str());
  EXPECT_FALSE(reference.isEmpty());

  reference.set("");
  EXPECT_TRUE(reference.isEmpty());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Reference, present)
{
  Reference   reference;

  reference.set("");
  reference.present("");

  reference.set("STR");
  reference.present("");
}



/* ****************************************************************************
*
* render - 
*/
TEST(Reference, render)
{
  Reference    reference;
  std::string  rendered;
  std::string  expected1 = "";
  std::string  expected2 = "<reference>REF</reference>\n";
  std::string  expected3 = "\"reference\" : \"REF\"\n";

  reference .set("");
  rendered = reference.render(XML, "", false);
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  reference .set("REF");
  rendered = reference.render(XML, "", false);
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = reference.render(JSON, "", false);
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* c_str - 
*/
TEST(Reference, c_str)
{
  Reference   reference;

  reference.set("STR");
  EXPECT_STREQ("STR", reference.c_str());
}
