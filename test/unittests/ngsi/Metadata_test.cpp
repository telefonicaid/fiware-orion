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

#include "ngsi/Metadata.h"



/* ****************************************************************************
*
* constructor - 
*/
TEST(Metadata, constructor)
{
   Metadata m1;
   Metadata m2("n2", "t2", "v2");
   Metadata m3(&m2);

   EXPECT_EQ("", m1.name);
   EXPECT_EQ("n2", m2.name);
   EXPECT_EQ("n2", m3.name);
}



/* ****************************************************************************
*
* render - 
*/
TEST(Metadata, render)
{
  std::string  rendered;
  Metadata     m1;
  Metadata     m2("Name", "Integer", "19");
  Metadata     m3("Name", "Association", "27");
  std::string  expected1 = "<contextMetadata>\n  <name></name>\n  <type></type>\n  <value></value>\n</contextMetadata>\n";
  std::string  expected2 = "\"contextMetadata\" : {\n  \"name\" : \"\",\n  \"type\" : \"\",\n  \"value\" : \"\"\n}\n";
  std::string  expected3 = "\"contextMetadata\" : {\n  \"name\" : \"Name\",\n  \"type\" : \"Integer\",\n  \"value\" : \"19\"\n}\n";
  std::string  expected4 = "<contextMetadata>\n  <name>Name</name>\n  <type>Integer</type>\n  <value>19</value>\n</contextMetadata>\n";
  std::string  expected5 = "<contextMetadata>\n  <name>Name</name>\n  <type>Association</type>\n  <value>\n    <entityAssociation>\n      <sourceEntityId type=\"\" isPattern=\"\">\n        <id></id>\n      </sourceEntityId>\n      <targetEntityId type=\"\" isPattern=\"\">\n        <id></id>\n      </targetEntityId>\n    </entityAssociation>\n</value>\n</contextMetadata>\n";
  std::string  expected6 = "\"contextMetadata\" : {\n  \"name\" : \"Name\",\n  \"type\" : \"Association\",\n  \"value\" : \n    \"entityAssociation\" : {\n      {\n        \"type\" : \"\",\n        \"isPattern\" : \"\",\n        \"id\" : \"\"\n      }\n      {\n        \"type\" : \"\",\n        \"isPattern\" : \"\",\n        \"id\" : \"\"\n      }\n    }\n\n}\n";

  rendered = m1.render(XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());
  rendered = m1.render(JSON, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());

  rendered = m2.render(JSON, "");
  EXPECT_STREQ(expected3.c_str(), rendered.c_str());
  rendered = m2.render(XML, "");
  EXPECT_STREQ(expected4.c_str(), rendered.c_str());

  rendered = m3.render(XML, "");
  EXPECT_STREQ(expected5.c_str(), rendered.c_str());
  rendered = m3.render(JSON, "");
  EXPECT_STREQ(expected6.c_str(), rendered.c_str());
}



/* ****************************************************************************
*
* check - 
*/
TEST(Metadata, check)
{
  Metadata     m1("", "Type", "Value");
  Metadata     m2("Name", "Type", "");
  Metadata     m3("Name", "Association", "XXX");
  Metadata     m4("Name", "Type", "Value");
  std::string  expected1 = "missing metadata name";
  std::string  expected2 = "missing metadata value";
  std::string  expected3 = "";
  std::string  expected4 = "OK";
  std::string  checked;

  checked = m1.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), checked.c_str());

  checked = m2.check(RegisterContext, JSON, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  checked = m3.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), checked.c_str());
  
  checked = m4.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected4.c_str(), checked.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Metadata, present)
{
  Metadata     m4("Name", "Type", "Value");

  m4.present("Test", 0, "");

  m4.release();
}
