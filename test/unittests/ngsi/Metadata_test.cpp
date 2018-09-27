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
#include "ngsi/Metadata.h"

#include "unittest.h"



/* ****************************************************************************
*
* constructor -
*/
TEST(Metadata, constructor)
{
  Metadata m1;
  Metadata m2("n2", "t2", "v2");
  Metadata m3(&m2);

  utInit();

  EXPECT_EQ("", m1.name);
  EXPECT_EQ("n2", m2.name);
  EXPECT_EQ("n2", m3.name);

  utExit();
}



/* ****************************************************************************
*
* render -
*
*/
TEST(Metadata, render)
{
  std::string  out;
  Metadata     m1;
  Metadata     m2("Name", "Integer", "19");

  const char*  outfile1 = "ngsi.metdata.render1.middle.json";
  const char*  outfile2 = "ngsi.metdata.render2.middle.json";

  utInit();

  out = m1.toJsonV1(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = m2.toJsonV1(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* check -
*/
TEST(Metadata, check)
{
  Metadata        m1("", "Type", "Value");
  Metadata        m2("Name", "Type", "");
  Metadata        m3("Name", "Type", "Value");
  std::string     checked;

  utInit();

  checked = m1.check(V1);
  EXPECT_STREQ("missing metadata name", checked.c_str());

  checked = m2.check(V1);
  EXPECT_STREQ("missing metadata value", checked.c_str());

  checked = m3.check(V1);
  EXPECT_STREQ("OK", checked.c_str());

  utExit();
}
