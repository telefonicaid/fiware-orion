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

#include "serviceRoutines/logTraceTreat.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "GET",    LogRequest,                            2, { "log", "traceLevel"                                  }, "", logTraceTreat                             },
  { "PUT",    LogRequest,                            3, { "log", "traceLevel", "*"                             }, "", logTraceTreat                             },
  { "POST",   LogRequest,                            3, { "log", "traceLevel", "*"                             }, "", logTraceTreat                             },
  { "DELETE", LogRequest,                            2, { "log", "traceLevel"                                  }, "", logTraceTreat                             },
  { "DELETE", LogRequest,                            3, { "log", "traceLevel", "*"                             }, "", logTraceTreat                             },

  { "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* get - 
*/
TEST(logTraceTreat, get)
{
  ConnectionInfo ci("/log/traceLevel",  "GET", "1.1");
  std::string    expected  = "<orion>\n  <log>\n    <tracelevels>empty</tracelevels>\n  </log>\n</orion>\n";
  std::string    out;

  lmTraceSet(NULL);

  ci.outFormat = XML;
  out          = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* put - 
*/
TEST(logTraceTreat, put)
{
  ConnectionInfo ci1("/log/traceLevel/0-19,21-200",  "PUT", "1.1");
  ConnectionInfo ci2("/log/traceLevel/aaa",  "PUT", "1.1");
  ConnectionInfo ci3("/log/traceLevel",  "GET", "1.1");
  std::string    expected1  = "<orion>\n  <log>\n    <tracelevels>0-19,21-200</tracelevels>\n  </log>\n</orion>\n";
  std::string    expected2  = "<orion>\n  <log>\n    <tracelevels>poorly formatted trace level string</tracelevels>\n  </log>\n</orion>\n";
  std::string    expected3  = "<orion>\n  <log>\n    <tracelevels>0-19, 21-200</tracelevels>\n  </log>\n</orion>\n";
  std::string    out;

  ci1.outFormat = XML;
  out          = restService(&ci1, rs);
  EXPECT_STREQ(expected1.c_str(), out.c_str());

  ci2.outFormat = XML;
  out          = restService(&ci2, rs);
  EXPECT_STREQ(expected2.c_str(), out.c_str());

  ci3.outFormat = XML;
  out          = restService(&ci3, rs);
  EXPECT_STREQ(expected3.c_str(), out.c_str());
}



/* ****************************************************************************
*
* post - 
*/
TEST(logTraceTreat, post)
{
  ConnectionInfo ci("/log/traceLevel/20",  "POST", "1.1");
  std::string    expected  = "<orion>\n  <log>\n    <bad URL/Verb>POST log/traceLevel/20</bad URL/Verb>\n  </log>\n</orion>\n";
  std::string    out;

  ci.outFormat = XML;
  out          = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* deleteIndividual - 
*/
TEST(logTraceTreat, deleteIndividual)
{
  ConnectionInfo ci1("/log/traceLevel/161",  "DELETE", "1.1");
  ConnectionInfo ci2("/log/traceLevel/aaa",  "DELETE", "1.1");
  ConnectionInfo ci3("/log/traceLevel",  "GET", "1.1");
  std::string    expected1  = "<orion>\n  <log>\n    <tracelevels removed>161</tracelevels removed>\n  </log>\n</orion>\n";
  std::string    expected2  = "<orion>\n  <log>\n    <tracelevels>poorly formatted trace level string</tracelevels>\n  </log>\n</orion>\n";
  std::string    expected3  = "<orion>\n  <log>\n    <tracelevels>0-19, 21-160, 162-200</tracelevels>\n  </log>\n</orion>\n";
  std::string    out;

  ci1.outFormat = XML;
  out          = restService(&ci1, rs);
  EXPECT_STREQ(expected1.c_str(), out.c_str());

  ci2.outFormat = XML;
  out          = restService(&ci2, rs);
  EXPECT_STREQ(expected2.c_str(), out.c_str());

  ci3.outFormat = XML;
  out          = restService(&ci3, rs);
  EXPECT_STREQ(expected3.c_str(), out.c_str());
}



/* ****************************************************************************
*
* deleteAll - 
*/
TEST(logTraceTreat, deleteAll)
{
  ConnectionInfo ci1("/log/traceLevel",  "DELETE", "1.1");
  ConnectionInfo ci2("/log/traceLevel",  "GET", "1.1");
  std::string    expected1  = "<orion>\n  <log>\n    <tracelevels>all trace levels off</tracelevels>\n  </log>\n</orion>\n";
  std::string    expected2  = "<orion>\n  <log>\n    <tracelevels>empty</tracelevels>\n  </log>\n</orion>\n";
  std::string    out;

  ci1.outFormat = XML;
  out           = restService(&ci1, rs);
  EXPECT_STREQ(expected1.c_str(), out.c_str());

  ci2.outFormat = XML;
  out           = restService(&ci2, rs);
  EXPECT_STREQ(expected2.c_str(), out.c_str());
}
