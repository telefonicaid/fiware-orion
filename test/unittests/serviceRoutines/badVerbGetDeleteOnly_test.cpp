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

#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "serviceRoutines/statisticsTreat.h"
#include "rest/RestService.h"
#include "rest/rest.h"



/* ****************************************************************************
*
* rs -
*/
static RestService getV[] =
{
  { StatisticsRequest, 1, { "statistics" }, statisticsTreat      },
  { InvalidRequest,    0, {              }, NULL                 }
};

static RestService deleteV[] =
{
  { StatisticsRequest, 1, { "statistics" }, statisticsTreat      },
  { InvalidRequest,    0, {              }, NULL                 }
};

static RestService badVerbV[] =
{
  { StatisticsRequest, 1, { "statistics" }, badVerbGetDeleteOnly },
  { InvalidRequest,    0, {              }, NULL                 }
};



/* ****************************************************************************
*
* ok -
*/
TEST(badVerbGetDeleteOnly, ok)
{
  ConnectionInfo  ci("/statistics",  "PUT", "1.1");
  std::string     expected = "";  // Bad verb gives no payload, only HTTP headers
  std::string     out;
  RestService     restService = { StatisticsRequest, 1, { "statistics" }, NULL };

  ci.apiVersion   = V1;
  ci.restServiceP = &restService;

  serviceVectorsSet(getV, NULL, NULL, NULL, deleteV, NULL, badVerbV);
  out = orion::requestServe(&ci);

  EXPECT_EQ(expected, out);
  EXPECT_EQ("Allow",       ci.httpHeader[0]);
  EXPECT_EQ("GET, DELETE", ci.httpHeaderValue[0]);
}
