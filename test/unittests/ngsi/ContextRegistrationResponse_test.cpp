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

#include "ngsi/ContextRegistrationResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* render - 
*/
TEST(ContextRegistrationResponse, render)
{
  ContextRegistrationResponse  crr;
  std::string                  rendered;
  const char*                  outfile1 = "ngsi.contextRegistrationResponse.renderOk.middle.json";
  const char*                  outfile2 = "ngsi.contextRegistrationResponse.renderError.middle.json";

  utInit();

  crr.errorCode.fill(SccNone);
  rendered = crr.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  crr.errorCode.fill(SccBadRequest);
  rendered = crr.render(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  utExit();
}



/* ****************************************************************************
*
* check - 
*/
TEST(ContextRegistrationResponse, check)
{
  ContextRegistrationResponse  crr;
  std::string                  checked;
  std::string                  expected = "no providing application";

  utInit();

  checked = crr.check(V1, RegisterContext, "", 0);
  EXPECT_STREQ(expected.c_str(), checked.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - no output expected, just exercising the code
*/
TEST(ContextRegistrationResponse, present)
{
  ContextRegistrationResponse  crr;

  utInit();

  crr.present("");

  utExit();
}
