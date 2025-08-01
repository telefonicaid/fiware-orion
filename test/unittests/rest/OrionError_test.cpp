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
  OrionError    e0;
  OrionError    e1;
  OrionError    e3;
  OrionError    e4;
  std::string   out;
  const char*   outfile1 = "orion.orionError.all1.valid.json";
  const char*   outfile2 = "orion.orionError.all3.valid.json";
  const char*   outfile3 = "orion.orionError.all4.valid.json";
  ConnectionInfo ci;

  e1.fill(SccOk, "no details 3");
  e3.fill(SccBadRequest, "no details 2");
  e4.fill(SccOk, "Good Request");

  ci.outMimeType = JSON;

  EXPECT_EQ(SccNone, e0.code);
  EXPECT_EQ("",      e0.error);
  EXPECT_EQ("",      e0.description);

  EXPECT_EQ(SccOk,          e1.code);
  EXPECT_EQ("OK",           e1.error);
  EXPECT_EQ("no details 3", e1.description);

  EXPECT_EQ(SccBadRequest,  e3.code);
  EXPECT_EQ("BadRequest",   e3.error);
  EXPECT_EQ("no details 2", e3.description);

  EXPECT_EQ(SccOk,          e4.code);
  EXPECT_EQ("OK",           e4.error);
  EXPECT_EQ("Good Request", e4.description);

  ci.outMimeType = JSON;

  out = e1.toJson();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = e3.toJson();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = e4.toJson();
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile3)) << "Error getting test data from '" << outfile3 << "'";

  EXPECT_STREQ(expectedBuf, out.c_str());
}
