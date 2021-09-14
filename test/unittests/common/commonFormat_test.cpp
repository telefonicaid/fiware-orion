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
#include "common/MimeType.h"


/* ****************************************************************************
*
* mimeTypeToString -
*/
TEST(commonMimeType, mimeTypeToString)
{
   char* mimeType;

   mimeType = (char*) mimeTypeToString(JSON);
   EXPECT_STREQ("JSON", mimeType) << "bad string translation for JSON mimeType";

   mimeType = (char*) mimeTypeToString(TEXT);
   EXPECT_STREQ("TEXT", mimeType) << "bad string translation for TEXT mimeType";

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
  EXPECT_EQ(JSON, mimeType);

  mimeType = stringToMimeType("noMimeType");
  EXPECT_EQ(NOMIMETYPE, mimeType);
}



/* ****************************************************************************
*
* mimeTypeParse -
*/
TEST(commonMimeType, mimeTypeParse)
{
   MimeType mimeType;

   mimeType = mimeTypeParse("*/*", NULL);
   EXPECT_EQ(JSON, mimeType) << "bad translation for JSON mimeType (*/*)";

   mimeType = mimeTypeParse("text/json", NULL);
   EXPECT_EQ(JSON, mimeType) << "bad translation for JSON mimeType (text/json)";

   mimeType = mimeTypeParse("application/json", NULL);
   EXPECT_EQ(JSON, mimeType) << "bad translation for JSON mimeType (application/json)";

   mimeType = mimeTypeParse("XXX", NULL);
   EXPECT_EQ(JSON, mimeType) << "bad translation for JSON mimeType (XXX - unknown )";

   std::string charset;
   mimeType = mimeTypeParse("application/json; charset=CHSET", &charset);
   EXPECT_EQ(JSON, mimeType) << "bad translation for JSON mimeType (application/json)";
   EXPECT_EQ("CHSET", charset);
}
