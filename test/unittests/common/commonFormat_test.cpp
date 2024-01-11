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
#include "orionld/types/OrionldMimeType.h"



/* ****************************************************************************
*
* mimeTypeToString -
*/
TEST(commonMimeType, mimeTypeToString)
{
   char* mimeType;

   mimeType = (char*) mimeTypeToString(MT_JSON);
   EXPECT_STREQ("JSON", mimeType) << "bad string translation for JSON mimeType";

   mimeType = (char*) mimeTypeToString(TEXT);
   EXPECT_STREQ("TEXT", mimeType) << "bad string translation for TEXT mimeType";

   mimeType = (char*) mimeTypeToString(HTML);
   EXPECT_STREQ("HTML", mimeType) << "bad string translation for HTML mimeType";

   mimeType = (char*) mimeTypeToString(NOMIMETYPE);
   EXPECT_STREQ("NOMIMETYPE", mimeType) << "bad string translation for NOMIMETYPE mimeType";

   mimeType = (char*) mimeTypeToString((MimeType) 0);
   EXPECT_STREQ("Unknown mimeType", mimeType) << "bad string translation for unknown mimeType";
}



/* ****************************************************************************
*
* stringTomimeType -
*/
TEST(commonMimeType, stringTomimeType)
{
  MimeType mimeType;

  mimeType = stringToMimeType("JSON");
  EXPECT_EQ(MT_JSON, mimeType);

  mimeType = stringToMimeType("noMimeType");
  EXPECT_EQ(NOMIMETYPE, mimeType);
}



extern MimeType contentTypeParse(const char* contentType, char** charsetP);
/* ****************************************************************************
*
* mimeTypeParse -
*/
TEST(commonMimeType, mimeTypeParse)
{
   MimeType mimeType;

   mimeType = contentTypeParse("*/*", NULL);
   EXPECT_EQ(MT_JSON, mimeType) << "bad translation for JSON mimeType (*/*)";

   mimeType = contentTypeParse("text/json", NULL);
   EXPECT_EQ(MT_JSON, mimeType) << "bad translation for JSON mimeType (text/json)";

   mimeType = contentTypeParse("application/json", NULL);
   EXPECT_EQ(MT_JSON, mimeType) << "bad translation for JSON mimeType (application/json)";

   mimeType = contentTypeParse("XXX", NULL);
   EXPECT_EQ(MT_JSON, mimeType) << "bad translation for JSON mimeType (XXX - unknown )";

   char* charset;
   mimeType = contentTypeParse("application/json; charset=CHSET", &charset);
   EXPECT_EQ(MT_JSON, mimeType) << "bad translation for JSON mimeType (application/json)";
   EXPECT_STREQ("CHSET", charset);
}
