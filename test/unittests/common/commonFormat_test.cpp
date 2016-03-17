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
#include "common/Format.h"


/* ****************************************************************************
*
* formatToString - 
*/
TEST(commonFormat, formatToString)
{
   char* format;

   format = (char*) formatToString(JSON);
   EXPECT_STREQ("JSON", format) << "bad string translation for JSON format";
   
   format = (char*) formatToString(TEXT);
   EXPECT_STREQ("TEXT", format) << "bad string translation for TEXT format";

   format = (char*) formatToString(HTML);
   EXPECT_STREQ("HTML", format) << "bad string translation for HTML format";

   format = (char*) formatToString(NOFORMAT);
   EXPECT_STREQ("NOFORMAT", format) << "bad string translation for NOFORMAT format";

   format = (char*) formatToString((Format) 0);
   EXPECT_STREQ("Unknown format", format) << "bad string translation for unknown format";
}



/* ****************************************************************************
*
* stringToformat - 
*/
TEST(commonFormat, stringToformat)
{
  Format format;

  format = stringToFormat("JSON");
  EXPECT_EQ(JSON, format);

  format = stringToFormat("noFormat");
  EXPECT_EQ(NOFORMAT, format);
}



/* ****************************************************************************
*
* formatParse - 
*/
TEST(commonFormat, formatParse)
{
   Format format;

   format = formatParse("*/*", NULL);
   EXPECT_EQ(JSON, format) << "bad translation for JSON format (*/*)";

   format = formatParse("text/json", NULL);
   EXPECT_EQ(JSON, format) << "bad translation for JSON format (text/json)";

   format = formatParse("application/json", NULL);
   EXPECT_EQ(JSON, format) << "bad translation for JSON format (application/json)";

   format = formatParse("XXX", NULL);
   EXPECT_EQ(JSON, format) << "bad translation for JSON format (XXX - unknown )";

   std::string charset;
   format = formatParse("application/json; charset=CHSET", &charset);
   EXPECT_EQ(JSON, format) << "bad translation for JSON format (application/json)";
   EXPECT_EQ("CHSET", charset);
}
