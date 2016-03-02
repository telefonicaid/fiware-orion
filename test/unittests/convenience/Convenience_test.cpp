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

#include "convenience/Convenience.h"

#include "common/Timer.h"
#include "common/globals.h"

#include "rest/RestService.h"
#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/getContextEntitiesByEntityId.h"

#include "unittest.h"



/* ****************************************************************************
*
* restServiceV - 
*/
RestService restServiceV[] =
{
  { "POST",   RegisterContext,                       2, { "ngsi9",  "registerContext"                       }, "", postRegisterContext                       },
  { "GET",    ContextEntitiesByEntityId,             3, { "ngsi9", "contextEntities", "*"                   }, "", getContextEntitiesByEntityId              },
  { "",       InvalidRequest,                        0, {                                                   }, "", NULL                                      }
};



/* ****************************************************************************
*
* emptyPath - 
*/
TEST(Convenience, emptyPath)
{
  ConnectionInfo            ci("", "GET", "1.1");
  std::string               response;
  std::string               expected = "Empty URL";

  utInit();

  response = restService(&ci, restServiceV);
  EXPECT_STREQ(expected.c_str(), response.c_str());

  utExit();
}



/* ****************************************************************************
*
* shortPath - 
*
* FIXME P5 #1862: _json countepart?
*/
TEST(Convenience, DISABLED_shortPath)
{
  ConnectionInfo  ci1("ngsi9", "GET", "1.1");
  ConnectionInfo  ci2("ngsi10", "GET", "1.1");
  ConnectionInfo  ci3("ngsi8", "GET", "1.1");
  ConnectionInfo  ci4("ngsi10/nada", "GET", "1.1");
  std::string     out;
  const char*     outfile1 = "ngsi.convenience.shortPath.postponed.xml";
  const char*     outfile2 = "ngsi.convenience.shortPath2.postponed.xml";
  const char*     outfile3 = "ngsi.convenience.shortPath3.postponed.xml";
  const char*     outfile4 = "ngsi.convenience.shortPath4.postponed.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  ci1.apiVersion = "v1";
  out = restService(&ci1, restServiceV);
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  ci2.apiVersion = "v1";
  out = restService(&ci2, restServiceV);
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  ci3.apiVersion = "v1";
  out = restService(&ci3, restServiceV);
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile4)) << "Error getting test data from '" << outfile4 << "'";
  ci4.apiVersion = "v1";
  out = restService(&ci4, restServiceV);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* badPathNgsi9 - 
*
* FIXME P5 #1862: _json countepart?
*/
TEST(Convenience, DISABLED_badPathNgsi9)
{
  ConnectionInfo            ci("ngsi9/badpathcomponent", "GET", "1.1");
  std::string               out;
  const char*               outfile = "ngsi.convenience.badPathNgsi9.postponed.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";

  ci.apiVersion = "v1";
  out = restService(&ci, restServiceV);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* badPathNgsi10 - 
*
* FIXME P5 #1862: _json countepart?
*/
TEST(Convenience, DISABLED_badPathNgsi10)
{
  ConnectionInfo            ci("ngsi10/badpathcomponent", "GET", "1.1");
  std::string               out;
  const char*               outfile = "ngsi10.convenience.badPathNgsi10.postponed.xml";

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  ci.apiVersion = "v1";
  out = restService(&ci, restServiceV);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
