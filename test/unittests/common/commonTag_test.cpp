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
   std::string      out;

   out = startTag(indent, tag, false);
   EXPECT_EQ("  \"TAG\" : {\n", out);

   out = startTag(indent, tag, true);
   EXPECT_EQ("  \"TAG\" : [\n", out);

   out = startTag(indent);
   EXPECT_EQ("  {\n", out);
}



/* ****************************************************************************
*
* endTag - 
*/
TEST(commonTag, endTag)
{
   std::string      indent = "  ";
   std::string      json   = "  }\n";
   std::string      out;

   out = endTag(indent);
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
   std::string      jsonComma               = "  \"TAG\" : \"tag\",\n";
   std::string      jsonNoComma             = "  \"TAG\" : \"tag\"\n";
   std::string      integerJsonNoComma      = "  \"TAG\" : \"8\"\n";
   std::string      stringJsonComma         = "  \"TAG\" : \"8\",\n";
   std::string      stringJsonNoComma       = "  \"TAG\" : \"8\"\n";
   std::string      out;

   out = valueTag(indent, tag, value);
   EXPECT_EQ(jsonNoComma, out);

   out = valueTag(indent, tag, value, true);
   EXPECT_EQ(jsonComma, out);   

   out = valueTag(indent, tag, value);
   EXPECT_EQ(jsonNoComma, out);   

   out = valueTag(indent, tag, 8, false);
   EXPECT_EQ(integerJsonNoComma, out);

   out = valueTag(indent, tag, "8", true, false);
   EXPECT_EQ(stringJsonComma, out);

   out = valueTag(indent, tag, "8", false, false);
   EXPECT_EQ(stringJsonNoComma, out);
}
