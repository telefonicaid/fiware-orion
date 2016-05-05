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
#include "common/Timer.h"
#include "common/globals.h"
#include "serviceRoutines/statisticsTreat.h"
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "GET",    StatisticsRequest, 1, { "statistics" }, "", statisticsTreat      },
  { "DELETE", StatisticsRequest, 1, { "statistics" }, "", statisticsTreat      },
  { "GET",    StatisticsRequest, 2, { "cache", "statistics" }, "", statisticsCacheTreat      },
  { "DELETE", StatisticsRequest, 2, { "cache", "statistics" }, "", statisticsCacheTreat      },
  { "*",      StatisticsRequest, 1, { "statistics" }, "", badVerbGetDeleteOnly },

  { "",       InvalidRequest,    0, {              }, "", NULL                 }
};



/* ****************************************************************************
*
* delete - 
*
* FIXME P5 #1862: _json countepart?
*/
TEST(statisticsTreat, DISABLED_delete)
{
  ConnectionInfo ci("/statistics",  "DELETE", "1.1");
  const char*    outfile1   = "orion.statistics.ok.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  out       = restService(&ci, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci.outMimeType = JSON;
  out            = restService(&ci, rs);
  EXPECT_STREQ("{\"message\":\"All statistics counter reset\"}", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* get - 
*
* FIXME P5 #1862: _json countepart?
*/
TEST(statisticsTreat, DISABLED_get)
{
  ConnectionInfo ci("/statistics",  "GET", "1.1");
  const char*    outfile1  = "orion.statistics.ok.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  out = restService(&ci, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci.outMimeType = JSON;  
  out            = restService(&ci, rs);
  EXPECT_STREQ("{\"uptime_in_secs\":0,\"measuring_interval_in_secs\":0}", out.c_str());

  utExit();
}

/* ****************************************************************************
*
* delete (cache) -
*
* FIXME P5 #1862: _json countepart?
*/
TEST(statisticsTreat, DISABLED_deleteCache)
{
  ConnectionInfo ci("/cache/statistics",  "DELETE", "1.1");
  const char*    outfile1   = "orion.statistics.ok.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  out       = restService(&ci, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci.outMimeType = JSON;
  out            = restService(&ci, rs);
  EXPECT_STREQ("{\"message\":\"All statistics counter reset\"}", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* get (cache) -
*
* FIXME P5 #1862: _json countepart?
*/
TEST(statisticsTreat, DISABLED_getCache)
{
  ConnectionInfo ci("/cache/statistics",  "GET", "1.1");
  const char*    outfile1  = "orion.statistics.ok.valid.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  out = restService(&ci, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci.outMimeType = JSON;
  out            = restService(&ci, rs);
  EXPECT_STREQ("{\"ids\":\"\",\"refresh\":0,\"inserts\":0,\"removes\":0,\"updates\":0,\"items\":0}", out.c_str());

  utExit();
}


/* ****************************************************************************
*
* badVerb - 
*/
TEST(statisticsTreat, badVerb)
{
  ConnectionInfo ci("/statistics",  "POLLUTE", "1.1");
  std::string    out;

  utInit();

  out = restService(&ci, rs);

  EXPECT_EQ("", out);
  EXPECT_EQ("Allow",        ci.httpHeader[0]);
  EXPECT_EQ("GET, DELETE",  ci.httpHeaderValue[0]);

  utExit();
}
