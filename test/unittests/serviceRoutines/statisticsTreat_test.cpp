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

#include "common/Timer.h"
#include "common/globals.h"
#include "serviceRoutines/statisticsTreat.h"
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "rest/RestService.h"
#include "rest/rest.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* service vectors -
*/
static RestService getV[] =
{
  { StatisticsRequest, 1, { "statistics"          }, "", statisticsTreat      },
  { StatisticsRequest, 2, { "cache", "statistics" }, "", statisticsCacheTreat },
  { InvalidRequest,    0, {                       }, "", NULL                 }
};

static RestService deleteV[] =
{
  { StatisticsRequest, 1, { "statistics"          }, "", statisticsTreat      },
  { StatisticsRequest, 2, { "cache", "statistics" }, "", statisticsCacheTreat },
  { InvalidRequest,    0, {                       }, "", NULL                 }
};

static RestService badVerbV[] =
{
  { StatisticsRequest, 1, { "statistics"          }, "", badVerbGetDeleteOnly },
  { InvalidRequest,    0, {                       }, "", NULL                 }
};



/* ****************************************************************************
*
* delete -
*/
TEST(statisticsTreat, delete)
{
  ConnectionInfo ci("/statistics",  "DELETE", "1.1");
  std::string    out;

  utInit();

  serviceVectorsSet(getV, NULL, NULL, NULL, deleteV, NULL, badVerbV);

  ci.outMimeType = JSON;
  out            = orion::requestServe(&ci);

  EXPECT_STREQ("{\"message\":\"All statistics counter reset\"}", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* get -
*/
TEST(statisticsTreat, get)
{
  ConnectionInfo ci("/statistics",  "GET", "1.1");
  std::string    out;

  utInit();

  serviceVectorsSet(getV, NULL, NULL, NULL, deleteV, NULL, badVerbV);

  ci.outMimeType = JSON;
  out            = orion::requestServe(&ci);

  EXPECT_STREQ("{\"uptime_in_secs\":0,\"measuring_interval_in_secs\":0}", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* delete (cache) -
*/
TEST(statisticsTreat, deleteCache)
{
  ConnectionInfo ci("/cache/statistics",  "DELETE", "1.1");
  std::string    out;

  utInit();

  serviceVectorsSet(getV, NULL, NULL, NULL, deleteV, NULL, badVerbV);

  ci.outMimeType = JSON;
  out            = orion::requestServe(&ci);

  EXPECT_STREQ("{\"message\":\"All statistics counter reset\"}", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* get (cache) -
*/
TEST(statisticsTreat, getCache)
{
  ConnectionInfo ci("/cache/statistics",  "GET", "1.1");
  std::string    out;

  utInit();

  serviceVectorsSet(getV, NULL, NULL, NULL, deleteV, NULL, badVerbV);

  ci.outMimeType = JSON;
  out            = orion::requestServe(&ci);

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

  serviceVectorsSet(getV, NULL, NULL, NULL, deleteV, NULL, badVerbV);

  out = orion::requestServe(&ci);

  EXPECT_EQ("", out);
  EXPECT_EQ("Allow",        ci.httpHeader[0]);
  EXPECT_EQ("GET, DELETE",  ci.httpHeaderValue[0]);

  utExit();
}
