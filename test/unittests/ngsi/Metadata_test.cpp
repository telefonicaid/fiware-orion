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
* FIXME P4 - extra newline at the end of expected3json
*/
TEST(Metadata, render)
{
  std::string  rendered;
  Metadata     m1;
  Metadata     m2("Name", "Integer", "19");
  Metadata     m3("Name", "Association", "27");

  const char*  outfile1 = "ngsi.metdata.render1.middle.xml";
  const char*  outfile2 = "ngsi.metdata.render1.middle.json";
  const char*  outfile3 = "ngsi.metdata.render2.middle.xml";
  const char*  outfile4 = "ngsi.metdata.render2.middle.json";
  const char*  outfile5 = "ngsi.metdata.render3.middle.xml";
  const char*  outfile6 = "ngsi.metdata.render3.middle.json";

  std::string  expected1xml  = "<contextMetadata>\n  <name></name>\n  <type></type>\n  <value></value>\n</contextMetadata>\n";
  std::string  expected1json = "{\n  \"name\" : \"\",\n  \"type\" : \"\",\n  \"value\" : \"\"\n}\n";
  std::string  expected2xml  = "<contextMetadata>\n  <name>Name</name>\n  <type>Integer</type>\n  <value>19</value>\n</contextMetadata>\n";
  std::string  expected2json = "{\n  \"name\" : \"Name\",\n  \"type\" : \"Integer\",\n  \"value\" : \"19\"\n}\n";
  std::string  expected3xml  = "<contextMetadata>\n  <name>Name</name>\n  <type>Association</type>\n  <value>\n    <entityAssociation>\n      <sourceEntityId type=\"\" isPattern=\"\">\n        <id></id>\n      </sourceEntityId>\n      <targetEntityId type=\"\" isPattern=\"\">\n        <id></id>\n      </targetEntityId>\n    </entityAssociation>\n</value>\n</contextMetadata>\n";
  std::string  expected3json = "{\n  \"name\" : \"Name\",\n  \"type\" : \"Association\",\n  \"value\" : \n    {\n      \"source\" : {\n        \"type\" : \"\",\n        \"isPattern\" : \"\",\n        \"id\" : \"\"\n      },\n      \"target\" : {\n        \"type\" : \"\",\n        \"isPattern\" : \"\",\n        \"id\" : \"\"\n      }\n    }\n\n}\n";

  utInit();

  rendered = m1.render(XML, "");
  EXPECT_STREQ(expected1xml.c_str(), rendered.c_str());
  rendered = m1.render(JSON, "");
  EXPECT_STREQ(expected1json.c_str(), rendered.c_str());

  rendered = m2.render(XML, "");
  EXPECT_STREQ(expected2xml.c_str(), rendered.c_str());
  rendered = m2.render(JSON, "");
  EXPECT_STREQ(expected2json.c_str(), rendered.c_str());

  rendered = m3.render(XML, "");
  EXPECT_STREQ(expected3xml.c_str(), rendered.c_str());
  rendered = m3.render(JSON, "");
  EXPECT_STREQ(expected3json.c_str(), rendered.c_str());

  utExit();
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

  utInit();

  checked = m1.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected1.c_str(), checked.c_str());

  checked = m2.check(RegisterContext, JSON, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  checked = m3.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), checked.c_str());
  
  checked = m4.check(RegisterContext, XML, "", "", 0);
  EXPECT_STREQ(expected4.c_str(), checked.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(Metadata, present)
{
  Metadata     m4("Name", "Type", "Value");

  utInit();

  m4.present("Test", 0, "");
  m4.release();

  utExit();
}
