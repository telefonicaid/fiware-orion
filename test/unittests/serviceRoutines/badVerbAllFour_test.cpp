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

#include "serviceRoutines/badVerbAllFour.h"
#include "rest/RestService.h"
#include "rest/rest.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* badVerbV -
*/
static RestService badVerbV[] =
{
  { IndividualContextEntity, 3, { "ngsi10", "contextEntities", "*" }, "", badVerbAllFour },
  { IndividualContextEntity, 2, { "ngsi10", "contextEntities"      }, "", badVerbAllFour },
  { IndividualContextEntity, 1, { "ngsi10"                         }, "", badVerbAllFour },
  { InvalidRequest,          0, {                                  }, "", NULL           }
};



/* ****************************************************************************
*
* error -
*/
TEST(badVerbAllFour, error)
{
  ConnectionInfo ci1("/ngsi10/contextEntities/123",  "PUST", "1.1");
  ConnectionInfo ci2("/ngsi10/contextEntities",      "PUST", "1.1");
  std::string    out;

  utInit();

  ci1.apiVersion = V1;

  serviceVectorsSet(NULL, NULL, NULL, NULL, NULL, NULL, badVerbV);
  out = orion::requestServe(&ci1);

  EXPECT_EQ("", out);
  EXPECT_EQ("Allow", ci1.httpHeader[0]);
  EXPECT_EQ("POST, GET, PUT, DELETE", ci1.httpHeaderValue[0]);

  ci2.apiVersion = V1;
  out = orion::requestServe(&ci2);
  EXPECT_EQ("", out);
  EXPECT_EQ("Allow", ci2.httpHeader[0]);
  EXPECT_EQ("POST, GET, PUT, DELETE", ci2.httpHeaderValue[0]);

  utExit();
}
