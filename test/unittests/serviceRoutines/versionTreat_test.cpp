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
#include <string>

#include "gtest/gtest.h"

#include "serviceRoutines/versionTreat.h"
#include "rest/RestService.h"
#include "rest/rest.h"



/* ****************************************************************************
*
* getV -
*/
static RestService getV[] =
{
  { VersionRequest, 1, { "version" }, versionTreat  },
  { InvalidRequest, 0, {           }, NULL          }
};



/* ****************************************************************************
*
* ok -
*/
TEST(versionTreat, ok)
{
  ConnectionInfo  ci("/version",  "GET", "1.1");
  std::string     out;
  RestService     restService = { VersionRequest, 1, { "version" }, NULL };

  ci.apiVersion   = V1;
  ci.restServiceP = &restService;

  serviceVectorsSet(getV, NULL, NULL, NULL, NULL, NULL, NULL);
  out = orion::requestServe(&ci);

  // FIXME P2: Some day we'll do this ...
  //
  // char*    expected =
  // "{\n"
  // "  \"orion\" : {\n"
  // "  \"version\" : \"" ORION_VERSION "\",\n"
  // "  \"uptime\" : \".*\",\n"
  // "  \"git_hash\" : \".*\",\n"
  // "  \"compile_time\" : \".*\",\n"
  // "  \"compiled_by\" : \".*\",\n"
  // "  \"compiled_in\" : \".*\"\n"
  // "  \"doc\" : \".*\"\n"
  // "  \"libversions\" : (drill down) "\n"
  // "}\n"
  // "}\n";
  // bool            match;
  // match = std::regex_match(expected, out.c_str());

  EXPECT_TRUE(strstr(out.c_str(), "orion") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "version") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "uptime") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "git_hash") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "compile_time") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "compiled_in") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "compiled_by") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "release_date") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "doc") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "libversions") != NULL);

  EXPECT_TRUE(strstr(out.c_str(), "boost") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "libcurl") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "libmicrohttpd") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "openssl") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "rapidjson") != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "mongodriver") != NULL);

  extern const char*  orionUnitTestVersion;
  std::string         expected = std::string("\"version\" : \"") + orionUnitTestVersion + "\"";
  EXPECT_TRUE(strstr(out.c_str(), expected.c_str()) != NULL);

  versionSet("1.2.3");
  out = orion::requestServe(&ci);
  EXPECT_TRUE(strstr(out.c_str(), "\"version\" : \"1.2.3\"") != NULL);

  versionSet("1.2.3");
  std::string version = versionGet();
  EXPECT_EQ("1.2.3", version);
}
