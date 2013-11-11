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

#include "ngsi/EntityId.h"



/* ****************************************************************************
*
* render - 
*/
TEST(EntityId, render)
{
  EntityId     eId;
  std::string  json;
  std::string  xml;
  std::string  jsonExpected = "\"type\" : \"\",\n\"isPattern\" : \"\",\n\"id\" : \"\"\n";
  std::string  xmlExpected  = "<eId type=\"\" isPattern=\"\">\n  <id></id>\n</eId>\n";

  eId.tagSet("eId");
  EXPECT_STREQ("eId", eId.tag.c_str());
  
  json = eId.render(JSON, "");
  EXPECT_STREQ(jsonExpected.c_str(), json.c_str());

  xml = eId.render(XML, "");
  EXPECT_STREQ(xmlExpected.c_str(), xml.c_str());
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(EntityId, present)
{
  EntityId     eId;

  eId.tagSet("entityId");
  eId.present("", -1);
  eId.present("", 0);
}
