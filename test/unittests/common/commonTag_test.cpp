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
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/tag.h"


/* ****************************************************************************
*
* startTag - 
*/
TEST(commonTag, startTag)
{
   std::string      tag    = "TAG";
   std::string      indent = "  ";
   std::string      xml    = "  <TAG>\n";
   std::string      json   = "  {\n";
   std::string      json2  = "  \"TAG\" : {\n";
   std::string      out;

   out = startTag(indent, tag, XML);
   EXPECT_EQ(xml, out);

   out = startTag(indent, tag, JSON, false);
   EXPECT_EQ(json, out);

   out = startTag(indent, tag, JSON, true);
   EXPECT_EQ(json2, out);

   out = startTag(indent, tag, HTML, true);
   EXPECT_EQ("Format not supported", out);

   out = startTag(indent, tag, tag, JSON, false, true);
   EXPECT_EQ("  \"TAG\" : {\n", out);

   out = startTag(indent, tag, tag, JSON, true, true);
   EXPECT_EQ("  \"TAG\" : [\n", out);

   out = startTag(indent, tag, tag, JSON, false, false);
   EXPECT_EQ("  {\n", out);

   out = startTag(indent, tag, tag, JSON, true, false);
   EXPECT_EQ("  [\n", out);

   out = startTag(indent, tag, tag, HTML, true, false);
   EXPECT_EQ("Format not supported", out);
}



/* ****************************************************************************
*
* endTag - 
*/
TEST(commonTag, endTag)
{
   std::string      tag    = "TAG";
   std::string      indent = "  ";
   std::string      xml    = "  </TAG>\n";
   std::string      json   = "  }\n";
   std::string      out;

   out = endTag(indent, tag, XML);
   EXPECT_EQ(xml, out);

   out = endTag(indent, tag, JSON);
   EXPECT_EQ(json, out);
}



/* ****************************************************************************
*
* valueTag - 
*/
TEST(commonTag, valueTag)
{
   std::string      tag                     = "TAG";
   std::string      indent                  = "  ";
   std::string      value                   = "tag";
   std::string      xml                     = "  <TAG>tag</TAG>\n";
   std::string      jsonComma               = "  \"TAG\" : \"tag\",\n";
   std::string      jsonNoComma             = "  \"TAG\" : \"tag\"\n";
   std::string      integerJsonNoComma      = "  \"TAG\" : \"8\"\n";
   std::string      stringJsonComma         = "  \"TAG\" : \"8\",\n";
   std::string      stringJsonNoComma       = "  \"TAG\" : \"8\"\n";
   std::string      out;

   out = valueTag(indent, tag, value, XML);
   EXPECT_EQ(xml, out);

   out = valueTag(indent, tag, value, JSON, true);
   EXPECT_EQ(jsonComma, out);   

   out = valueTag(indent, tag, value, JSON);
   EXPECT_EQ(jsonNoComma, out);   

   out = valueTag(indent, tag, 8, JSON, false);
   EXPECT_EQ(integerJsonNoComma, out);

   out = valueTag(indent, tag, tag, "8", JSON, true);
   EXPECT_EQ(stringJsonComma, out);

   out = valueTag(indent, tag, tag, "8", JSON, false);
   EXPECT_EQ(stringJsonNoComma, out);
}
