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

#include "serviceRoutines/logVerboseTreat.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "GET LogRequest",                             "GET",    LogRequest,                            2, { "log", "verbose"                                     }, "", logVerboseTreat                           },
  { "PUT LogRequest",                             "PUT",    LogRequest,                            3, { "log", "verbose", "*"                                }, "", logVerboseTreat                           },
  { "POST LogRequest",                            "POST",   LogRequest,                            3, { "log", "verbose", "*"                                }, "", logVerboseTreat                           },
  { "DELETE LogRequest",                          "DELETE", LogRequest,                            2, { "log", "verbose"                                     }, "", logVerboseTreat                           },

  { "* InvalidRequest",                           "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "* *",                                        "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* get - 
*/
TEST(logVerboseTreat, get)
{
  ConnectionInfo ci("/log/verbose",  "GET", "1.1");
  std::string    expected  = "<orion>\n  <log>\n    <verbose level>0</verbose level>\n  </log>\n</orion>\n";
  std::string    out;

  ci.outFormat = XML;
  out          = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* put - 
*/
TEST(logVerboseTreat, put)
{
  ConnectionInfo ci1("/log/verbose/5",  "PUT", "1.1");
  ConnectionInfo ci2("/log/verbose/19", "PUT", "1.1");
  ConnectionInfo ci3("/log/verbose",    "GET", "1.1");
  std::string    expected1  = "<orion>\n  <log>\n    <verbose level>verbose level set to 5</verbose level>\n  </log>\n</orion>\n";
  std::string    expected2  = "<orion>\n  <log>\n    <verbose level>bad URL or Verb</verbose level>\n  </log>\n</orion>\n";
  std::string    expected3  = "<orion>\n  <log>\n    <verbose level>5</verbose level>\n  </log>\n</orion>\n";
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
TEST(logVerboseTreat, post)
{
  ConnectionInfo ci("/log/verbose/2",  "POST", "1.1");
  std::string    expected  = "<orion>\n  <log>\n    <verbose level>bad URL or Verb</verbose level>\n  </log>\n</orion>\n";
  std::string    out;

  ci.outFormat = XML;
  out          = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* delete - 
*/
TEST(logVerboseTreat, delete)
{
  ConnectionInfo ci1("/log/verbose",  "DELETE", "1.1");
  ConnectionInfo ci2("/log/verbose",  "GET", "1.1");
  std::string    expected1  = "<orion>\n  <log>\n    <verbose level>all verbose levels reset</verbose level>\n  </log>\n</orion>\n";
  std::string    expected2  = "<orion>\n  <log>\n    <verbose level>0</verbose level>\n  </log>\n</orion>\n";
  std::string    out;


  ci1.outFormat = XML;
  out          = restService(&ci1, rs);
  EXPECT_STREQ(expected1.c_str(), out.c_str());

  ci2.outFormat = XML;
  out          = restService(&ci2, rs);
  EXPECT_STREQ(expected2.c_str(), out.c_str());
}



/* ****************************************************************************
*
* put2 - 
*/
TEST(logVerboseTreat, put2)
{
  ConnectionInfo ci01("/log/verbose/5",   "PUT", "1.1");
  ConnectionInfo ci02("/log/verbose",     "GET", "1.1");
  ConnectionInfo ci03("/log/verbose/off", "PUT", "1.1");
  ConnectionInfo ci04("/log/verbose",     "GET", "1.1");
  ConnectionInfo ci05("/log/verbose/0",   "PUT", "1.1");
  ConnectionInfo ci06("/log/verbose",     "GET", "1.1");
  ConnectionInfo ci07("/log/verbose/0",   "PUT", "1.1");
  ConnectionInfo ci08("/log/verbose",     "GET", "1.1");
  ConnectionInfo ci09("/log/verbose/1",   "PUT", "1.1");
  ConnectionInfo ci10("/log/verbose",     "GET", "1.1");
  ConnectionInfo ci11("/log/verbose/2",   "PUT", "1.1");
  ConnectionInfo ci12("/log/verbose",     "GET", "1.1");
  ConnectionInfo ci13("/log/verbose/3",   "PUT", "1.1");
  ConnectionInfo ci14("/log/verbose",     "GET", "1.1");
  ConnectionInfo ci15("/log/verbose/4",   "PUT", "1.1");
  ConnectionInfo ci16("/log/verbose",     "GET", "1.1");

  std::string    expected01  = "<orion>\n  <log>\n    <verbose level>verbose level set to 5</verbose level>\n  </log>\n</orion>\n";
  std::string    expected02  = "<orion>\n  <log>\n    <verbose level>5</verbose level>\n  </log>\n</orion>\n";
  std::string    expected03  = "<orion>\n  <log>\n    <verbose level>all verbose levels reset</verbose level>\n  </log>\n</orion>\n";
  std::string    expected04  = "<orion>\n  <log>\n    <verbose level>0</verbose level>\n  </log>\n</orion>\n";
  std::string    expected05  = "<orion>\n  <log>\n    <verbose level>all verbose levels reset</verbose level>\n  </log>\n</orion>\n";
  std::string    expected06  = "<orion>\n  <log>\n    <verbose level>0</verbose level>\n  </log>\n</orion>\n";
  std::string    expected07  = "<orion>\n  <log>\n    <verbose level>all verbose levels reset</verbose level>\n  </log>\n</orion>\n";
  std::string    expected08  = "<orion>\n  <log>\n    <verbose level>0</verbose level>\n  </log>\n</orion>\n";
  std::string    expected09  = "<orion>\n  <log>\n    <verbose level>verbose level set to 1</verbose level>\n  </log>\n</orion>\n";
  std::string    expected10  = "<orion>\n  <log>\n    <verbose level>1</verbose level>\n  </log>\n</orion>\n";
  std::string    expected11  = "<orion>\n  <log>\n    <verbose level>verbose level set to 2</verbose level>\n  </log>\n</orion>\n";
  std::string    expected12  = "<orion>\n  <log>\n    <verbose level>2</verbose level>\n  </log>\n</orion>\n";
  std::string    expected13  = "<orion>\n  <log>\n    <verbose level>verbose level set to 3</verbose level>\n  </log>\n</orion>\n";
  std::string    expected14  = "<orion>\n  <log>\n    <verbose level>3</verbose level>\n  </log>\n</orion>\n";
  std::string    expected15  = "<orion>\n  <log>\n    <verbose level>verbose level set to 4</verbose level>\n  </log>\n</orion>\n";
  std::string    expected16  = "<orion>\n  <log>\n    <verbose level>4</verbose level>\n  </log>\n</orion>\n";
  std::string    out;


  ci01.outFormat = XML;
  out          = restService(&ci01, rs);
  EXPECT_STREQ(expected01.c_str(), out.c_str());

  ci02.outFormat = XML;
  out          = restService(&ci02, rs);
  EXPECT_STREQ(expected02.c_str(), out.c_str());

  ci03.outFormat = XML;
  out          = restService(&ci03, rs);
  EXPECT_STREQ(expected03.c_str(), out.c_str());

  ci04.outFormat = XML;
  out          = restService(&ci04, rs);
  EXPECT_STREQ(expected04.c_str(), out.c_str());

  ci05.outFormat = XML;
  out          = restService(&ci05, rs);
  EXPECT_STREQ(expected05.c_str(), out.c_str());

  ci06.outFormat = XML;
  out          = restService(&ci06, rs);
  EXPECT_STREQ(expected06.c_str(), out.c_str());

  ci07.outFormat = XML;
  out          = restService(&ci07, rs);
  EXPECT_STREQ(expected07.c_str(), out.c_str());

  ci08.outFormat = XML;
  out          = restService(&ci08, rs);
  EXPECT_STREQ(expected08.c_str(), out.c_str());

  ci09.outFormat = XML;
  out          = restService(&ci09, rs);
  EXPECT_STREQ(expected09.c_str(), out.c_str());

  ci10.outFormat = XML;
  out          = restService(&ci10, rs);
  EXPECT_STREQ(expected10.c_str(), out.c_str());

  ci11.outFormat = XML;
  out          = restService(&ci11, rs);
  EXPECT_STREQ(expected11.c_str(), out.c_str());

  ci12.outFormat = XML;
  out          = restService(&ci12, rs);
  EXPECT_STREQ(expected12.c_str(), out.c_str());

  ci13.outFormat = XML;
  out          = restService(&ci13, rs);
  EXPECT_STREQ(expected13.c_str(), out.c_str());

  ci14.outFormat = XML;
  out          = restService(&ci14, rs);
  EXPECT_STREQ(expected14.c_str(), out.c_str());

  ci15.outFormat = XML;
  out          = restService(&ci15, rs);
  EXPECT_STREQ(expected15.c_str(), out.c_str());

  ci16.outFormat = XML;
  out          = restService(&ci16, rs);
  EXPECT_STREQ(expected16.c_str(), out.c_str());
}
