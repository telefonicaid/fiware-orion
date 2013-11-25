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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include "gtest/gtest.h"

#include "serviceRoutines/badVerbAllFour.h"
#include "rest/RestService.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
   { "* IndividualContextEntity",                  "*",      IndividualContextEntity,               3, { "ngsi10", "contextEntities", "*"                         }, "", badVerbAllFour                            },
   { "* IndividualContextEntity",                  "*",      IndividualContextEntity,               2, { "ngsi10", "contextEntities"                              }, "", badVerbAllFour                            },
   { "* IndividualContextEntity",                  "*",      IndividualContextEntity,               1, { "ngsi10"                                                 }, "", badVerbAllFour                            },
   { "* *",                                        "",       InvalidRequest,                        0, {                                                          }, "", NULL                                      }
};



/* ****************************************************************************
*
* error - 
*/
TEST(badVerbAllFour, error)
{
  ConnectionInfo ci1("/ngsi10/contextEntities/123",  "PUST", "1.1");
  ConnectionInfo ci2("/ngsi10/contextEntities",      "PUST", "1.1");
  ConnectionInfo ci3("/ngsi10/",                     "PUST", "1.1");
  ConnectionInfo ci4("/ngsi10/1/2/3/4",              "PUST", "1.1");
  std::string    expected  = "";  // Bad verb gives no payload, only HTTP headers
  std::string    expected3 = "<orionError>\n  <code>400</code>\n  <reasonPhrase>bad request</reasonPhrase>\n  <details>Service not recognized</details>\n</orionError>\n";
  std::string    out;

  out = restService(&ci1, rs);
  EXPECT_EQ(expected, out);
  EXPECT_EQ("Allow", ci1.httpHeader[0]);
  EXPECT_EQ("POST, GET, PUT, DELETE", ci1.httpHeaderValue[0]);

  out = restService(&ci2, rs);
  EXPECT_EQ(expected, out);
  EXPECT_EQ("Allow", ci2.httpHeader[0]);
  EXPECT_EQ("POST, GET, PUT, DELETE", ci2.httpHeaderValue[0]);

  out = restService(&ci3, rs);
  EXPECT_EQ(expected3, out);
  out = restService(&ci4, rs);
  EXPECT_EQ(expected3, out);
}
