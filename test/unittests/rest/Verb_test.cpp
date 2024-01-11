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
#include "rest/Verb.h"


/* ****************************************************************************
*
* all -
*/
TEST(Verb, all)
{
  EXPECT_STREQ("GET",          verbName(HTTP_GET));
  EXPECT_STREQ("PUT",          verbName(HTTP_PUT));
  EXPECT_STREQ("POST",         verbName(HTTP_POST));
  EXPECT_STREQ("DELETE",       verbName(HTTP_DELETE));
  EXPECT_STREQ("PATCH",        verbName(HTTP_PATCH));
  EXPECT_STREQ("HEAD",         verbName(HTTP_HEAD));
  EXPECT_STREQ("OPTIONS",      verbName(HTTP_OPTIONS));
  EXPECT_STREQ("TRACE",        verbName(HTTP_TRACE));
  EXPECT_STREQ("CONNECT",      verbName(HTTP_CONNECT));
  EXPECT_STREQ("",             verbName(HTTP_NOVERB));
  EXPECT_STREQ("Unknown verb", verbName((Verb) 1234));
}
