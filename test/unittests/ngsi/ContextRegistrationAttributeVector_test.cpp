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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ContextRegistrationAttributeVector.h"

#include "unittest.h"



/* ****************************************************************************
*
* render -
*/
TEST(ContextRegistrationAttributeVector, render)
{
  ContextRegistrationAttributeVector crav;
  ContextRegistrationAttribute       cra("name", "type", "false");
  ContextRegistrationAttribute       cra2("name2", "type2", "true");
  std::string                        out;
  const char*                        outfile1 = "ngsi.contextRegistrationAttributeVector.render1.middle.json";
  const char*                        outfile2 = "ngsi.contextRegistrationAttributeVector.render2.middle.json";

  utInit();

  out = crav.render(false);
  EXPECT_STREQ("", out.c_str());

  out = crav.render(false);
  EXPECT_STREQ("", out.c_str());

  crav.push_back(&cra);
  out = crav.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  crav.push_back(&cra2);
  out = crav.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
