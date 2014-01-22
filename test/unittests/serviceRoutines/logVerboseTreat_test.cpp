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
#include "serviceRoutines/logVerboseTreat.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "GET",    LogRequest,                            2, { "log", "verbose"                                     }, "", logVerboseTreat                           },
  { "PUT",    LogRequest,                            3, { "log", "verbose", "*"                                }, "", logVerboseTreat                           },
  { "POST",   LogRequest,                            3, { "log", "verbose", "*"                                }, "", logVerboseTreat                           },
  { "DELETE", LogRequest,                            2, { "log", "verbose"                                     }, "", logVerboseTreat                           },

  { "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* get - 
*/
TEST(logVerboseTreat, get)
{
  ConnectionInfo ci("/log/verbose",  "GET", "1.1");
  const char*    outfile = "orion.logVerbose.get0.valid.xml";
  std::string    out;

  utInit();

  ci.outFormat = XML;
  out          = restService(&ci, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
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
  const char*    outfile1 = "orion.logVerbose.put.5.valid.xml";
  const char*    outfile2 = "orion.logVerbose.put.19.valid.xml";
  const char*    outfile3 = "orion.logVerbose.get.5.valid.xml";
  std::string    out;

  utInit();

  ci1.outFormat = XML;
  out          = restService(&ci1, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci2.outFormat = XML;
  out          = restService(&ci2, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci3.outFormat = XML;
  out          = restService(&ci3, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* post - 
*/
TEST(logVerboseTreat, post)
{
  ConnectionInfo ci("/log/verbose/2",  "POST", "1.1");
  const char*    outfile = "orion.logVerbose.post.2.valid.xml";
  std::string    out;

  utInit();

  ci.outFormat = XML;
  out          = restService(&ci, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* delete - 
*/
TEST(logVerboseTreat, delete)
{
  ConnectionInfo ci1("/log/verbose",  "DELETE", "1.1");
  ConnectionInfo ci2("/log/verbose",  "GET", "1.1");
  const char*    outfile1 = "orion.logVerbose.delete.delete.valid.xml";
  const char*    outfile2 = "orion.logVerbose.delete.get.valid.xml";
  std::string    out;

  utInit();

  ci1.outFormat = XML;
  out          = restService(&ci1, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci2.outFormat = XML;
  out          = restService(&ci2, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
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
  const char*    outfile01 = "orion.logVerbose.put2-01.valid.xml";
  const char*    outfile02 = "orion.logVerbose.put2-02.valid.xml";
  const char*    outfile03 = "orion.logVerbose.put2-03.valid.xml";
  const char*    outfile04 = "orion.logVerbose.put2-04.valid.xml";
  const char*    outfile05 = "orion.logVerbose.put2-05.valid.xml";
  const char*    outfile06 = "orion.logVerbose.put2-06.valid.xml";
  const char*    outfile07 = "orion.logVerbose.put2-07.valid.xml";
  const char*    outfile08 = "orion.logVerbose.put2-08.valid.xml";
  const char*    outfile09 = "orion.logVerbose.put2-09.valid.xml";
  const char*    outfile10 = "orion.logVerbose.put2-10.valid.xml";
  const char*    outfile11 = "orion.logVerbose.put2-11.valid.xml";
  const char*    outfile12 = "orion.logVerbose.put2-12.valid.xml";
  const char*    outfile13 = "orion.logVerbose.put2-13.valid.xml";
  const char*    outfile14 = "orion.logVerbose.put2-14.valid.xml";
  const char*    outfile15 = "orion.logVerbose.put2-15.valid.xml";
  const char*    outfile16 = "orion.logVerbose.put2-16.valid.xml";
  std::string    out;

  utInit();

  ci01.outFormat = XML;
  out          = restService(&ci01, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile01)) << "Error getting test data from '" << outfile01 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci02.outFormat = XML;
  out          = restService(&ci02, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile02)) << "Error getting test data from '" << outfile02 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci03.outFormat = XML;
  out          = restService(&ci03, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile03)) << "Error getting test data from '" << outfile03 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci04.outFormat = XML;
  out          = restService(&ci04, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile04)) << "Error getting test data from '" << outfile04 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci05.outFormat = XML;
  out          = restService(&ci05, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile05)) << "Error getting test data from '" << outfile05 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci06.outFormat = XML;
  out          = restService(&ci06, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile06)) << "Error getting test data from '" << outfile06 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci07.outFormat = XML;
  out          = restService(&ci07, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile07)) << "Error getting test data from '" << outfile07 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci08.outFormat = XML;
  out          = restService(&ci08, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile08)) << "Error getting test data from '" << outfile08 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci09.outFormat = XML;
  out          = restService(&ci09, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile09)) << "Error getting test data from '" << outfile09 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci10.outFormat = XML;
  out          = restService(&ci10, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile10)) << "Error getting test data from '" << outfile10 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci11.outFormat = XML;
  out          = restService(&ci11, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile11)) << "Error getting test data from '" << outfile11 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci12.outFormat = XML;
  out          = restService(&ci12, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile12)) << "Error getting test data from '" << outfile12 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci13.outFormat = XML;
  out          = restService(&ci13, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile13)) << "Error getting test data from '" << outfile13 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci14.outFormat = XML;
  out          = restService(&ci14, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile14)) << "Error getting test data from '" << outfile14 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci15.outFormat = XML;
  out          = restService(&ci15, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile15)) << "Error getting test data from '" << outfile15 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci16.outFormat = XML;
  out          = restService(&ci16, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile16)) << "Error getting test data from '" << outfile16 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
