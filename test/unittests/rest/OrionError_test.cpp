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

#include "logMsg/logMsg.h"

#include "rest/OrionError.h"
#include "rest/ConnectionInfo.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* all -
*/
TEST(OrionError, all)
{
  StatusCode    sc(SccBadRequest, "no details 2");
  OrionError    e0;
  OrionError    e1(SccOk, "no details 3");
  OrionError    e3(sc);
  OrionError    e4(SccOk, "Good Request");
  std::string   out;
  const char*   outfile1 = "orion.orionError.all1.valid.json";
  const char*   outfile2 = "orion.orionError.all3.valid.json";
  const char*   outfile3 = "orion.orionError.all4.valid.json";
  ConnectionInfo ci;

  ci.outMimeType = JSON;

  EXPECT_EQ(SccNone, e0.code);
  EXPECT_EQ("",      e0.reasonPhrase);
  EXPECT_EQ("",      e0.details);

  EXPECT_EQ(SccOk,          e1.code);
  EXPECT_EQ("OK",           e1.reasonPhrase);
  EXPECT_EQ("no details 3", e1.details);

  EXPECT_EQ(sc.code,         e3.code);
  EXPECT_EQ(sc.reasonPhrase, e3.reasonPhrase);
  EXPECT_EQ(sc.details,      e3.details);

  EXPECT_EQ(SccOk,          e4.code);
  EXPECT_EQ("OK",           e4.reasonPhrase);
  EXPECT_EQ("Good Request", e4.details);

  ci.outMimeType = JSON;

  out = e1.render();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = e3.render();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = e4.render();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile3)) << "Error getting test data from '" << outfile3 << "'";

  EXPECT_STREQ(expectedBuf, out.c_str());
}
