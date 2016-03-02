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
#include "serviceRoutines/leakTreat.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "GET",    LeakRequest,                           2, { "leak", "*"                                              }, "", leakTreat                                 },
  { "GET",    LeakRequest,                           1, { "leak"                                                   }, "", leakTreat                                 },
  { "",       InvalidRequest,                        0, {                                                          }, "", NULL                                      }
};



/* ****************************************************************************
*
* error - 
*
* FIXME P5 #1862: _json countepart?
*/
extern bool harakiri;
TEST(leakTreat, DISABLED_error)
{
  ConnectionInfo ci1("/leak",  "GET", "1.1");
  ConnectionInfo ci2("/leak/nadadenada",  "GET", "1.1");
  ConnectionInfo ci3("/leak/harakiri",  "GET", "1.1");
  const char*    outfile1 = "orion.leak.passwordRequested.valid.xml";
  const char*    outfile2 = "orion.leak.passwordErroneous.valid.xml";
  const char*    outfile3 = "orion.leak.noSuchService.valid.xml";
  const char*    outfile4 = "orion.leak.ok.valid.xml";
  std::string    out;

  utInit();

  harakiri = true;

  ci1.apiVersion = "v1";
  out = restService(&ci1, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ci2.apiVersion = "v1";
  out = restService(&ci2, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  harakiri = false;
  ci3.apiVersion = "v1";
  out       = restService(&ci3, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile3)) << "Error getting test data from '" << outfile3 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  harakiri = true;
  out       = restService(&ci3, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile4)) << "Error getting test data from '" << outfile4 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  harakiri = false;

  utExit();
}
