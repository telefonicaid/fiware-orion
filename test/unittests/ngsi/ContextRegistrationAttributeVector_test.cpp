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

#include "ngsi/ContextRegistrationAttributeVector.h"



/* ****************************************************************************
*
* render - 
*/
TEST(ContextRegistrationAttributeVector, render)
{
  ContextRegistrationAttributeVector crav;
  ContextRegistrationAttribute       cra("name", "type", "false");
  ContextRegistrationAttribute       cra2("name2", "type2", "true");
  std::string                        out;
  std::string                        expected1xml  = "<contextRegistrationAttributeList>\n  <contextRegistrationAttribute>\n    <name>name</name>\n    <type>type</type>\n    <isDomain>false</isDomain>\n  </contextRegistrationAttribute>\n</contextRegistrationAttributeList>\n";
  std::string                        expected1json = "\"attributes\" : [\n  {\n    \"name\" : \"name\",\n    \"type\" : \"type\",\n    \"isDomain\" : \"false\"\n  }\n]\n";
  std::string                        expected2xml  = "<contextRegistrationAttributeList>\n  <contextRegistrationAttribute>\n    <name>name</name>\n    <type>type</type>\n    <isDomain>false</isDomain>\n  </contextRegistrationAttribute>\n  <contextRegistrationAttribute>\n    <name>name2</name>\n    <type>type2</type>\n    <isDomain>true</isDomain>\n  </contextRegistrationAttribute>\n</contextRegistrationAttributeList>\n";
  std::string                        expected2json = "\"attributes\" : [\n  {\n    \"name\" : \"name\",\n    \"type\" : \"type\",\n    \"isDomain\" : \"false\"\n  },\n  {\n    \"name\" : \"name2\",\n    \"type\" : \"type2\",\n    \"isDomain\" : \"true\"\n  }\n]\n";

  out = crav.render(XML, "");
  EXPECT_STREQ("", out.c_str());

  out = crav.render(JSON, "");
  EXPECT_STREQ("", out.c_str());

  crav.push_back(&cra);
  out = crav.render(XML, "");
  EXPECT_STREQ(expected1xml.c_str(), out.c_str());
  out = crav.render(JSON, "");
  EXPECT_STREQ(expected1json.c_str(), out.c_str());

  crav.push_back(&cra2);
  out = crav.render(XML, "");
  EXPECT_STREQ(expected2xml.c_str(), out.c_str());
  out = crav.render(JSON, "");
  EXPECT_STREQ(expected2json.c_str(), out.c_str());
}
