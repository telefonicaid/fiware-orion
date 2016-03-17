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
#include "serviceRoutines/badVerbAllFour.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
   { "*",      IndividualContextEntity,               3, { "ngsi10", "contextEntities", "*"                         }, "", badVerbAllFour                            },
   { "*",      IndividualContextEntity,               2, { "ngsi10", "contextEntities"                              }, "", badVerbAllFour                            },
   { "*",      IndividualContextEntity,               1, { "ngsi10"                                                 }, "", badVerbAllFour                            },
   { "",       InvalidRequest,                        0, {                                                          }, "", NULL                                      }
};



/* ****************************************************************************
*
* error - 
*
* FIXME P5 #1862: _json counterpart?
*/
TEST(badVerbAllFour, DISABLED_error)
{
  ConnectionInfo ci1("/ngsi10/contextEntities/123",  "PUST", "1.1");
  ConnectionInfo ci2("/ngsi10/contextEntities",      "PUST", "1.1");
  ConnectionInfo ci3("/ngsi10/",                     "PUST", "1.1");
  ConnectionInfo ci4("/ngsi10/1/2/3/4",              "PUST", "1.1");
  const char*    outfile1 = "orion.serviceNotRecognized.valid.xml";
  const char*    outfile2 = "orion.serviceNotRecognized2.valid.xml";
  std::string    out;

  utInit();

  ci1.apiVersion = "v1";
  out = restService(&ci1, rs);
  EXPECT_EQ("", out);
  EXPECT_EQ("Allow", ci1.httpHeader[0]);
  EXPECT_EQ("POST, GET, PUT, DELETE", ci1.httpHeaderValue[0]);

  ci2.apiVersion = "v1";
  out = restService(&ci2, rs);
  EXPECT_EQ("", out);
  EXPECT_EQ("Allow", ci2.httpHeader[0]);
  EXPECT_EQ("POST, GET, PUT, DELETE", ci2.httpHeaderValue[0]);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  ci3.apiVersion = "v1";
  out = restService(&ci3, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  ci4.apiVersion = "v1";
  out = restService(&ci4, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
