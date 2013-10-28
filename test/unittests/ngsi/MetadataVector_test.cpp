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

#include "ngsi/MetadataVector.h"



/* ****************************************************************************
*
* render - 
*/
TEST(MetadataVector, render)
{
  Metadata        m("Name", "Type", "Value");
  MetadataVector  mV("MV");
  std::string     expected1 = "<MV>\n  <contextMetadata>\n    <name>Name</name>\n    <type>Type</type>\n    <value>Value</value>\n  </contextMetadata>\n</MV>\n";
  std::string     expected2 = "<mv>\n  <contextMetadata>\n    <name>Name</name>\n    <type>Type</type>\n    <value>Value</value>\n  </contextMetadata>\n</mv>\n";
  std::string     rendered;

  mV.push_back(&m);

  rendered = mV.render(XML, "");
  EXPECT_STREQ(expected1.c_str(), rendered.c_str());

  mV.tagSet("mv");
  rendered = mV.render(XML, "");
  EXPECT_STREQ(expected2.c_str(), rendered.c_str());
}
