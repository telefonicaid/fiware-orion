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

#include "ngsi/NotifyConditionVector.h"

#include "unittest.h"



/* ****************************************************************************
*
* render -
*/
TEST(NotifyConditionVector, render)
{
  NotifyCondition*       ncP = new NotifyCondition();
  NotifyConditionVector  ncV;
  std::string            out;
  const char*            outfile1 = "ngsi.notifyConditionVector.render.middle.json";

  utInit();

  out = ncV.toJsonV1(false);
  EXPECT_STREQ("", out.c_str());

  ncP->type = "Type";
  ncV.push_back(ncP);

  out = ncV.toJsonV1(false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  ncV.release();

  out = ncV.toJsonV1(false);
  EXPECT_STREQ("", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* check -
*/
TEST(NotifyConditionVector, check)
{
  NotifyCondition        nc;
  NotifyConditionVector  ncV;
  std::string            checked;
  std::string            expected2 = "invalid notify condition type: /Type/";
  std::string            expected3 = "empty type for NotifyCondition";

  utInit();

  checked = ncV.check(RegisterContext, "", 0);
  EXPECT_STREQ("OK", checked.c_str());

  nc.type = "Type";
  ncV.push_back(&nc);

  checked = ncV.check(RegisterContext, "", 0);
  EXPECT_STREQ(expected2.c_str(), checked.c_str());

  nc.type = "";
  checked = ncV.check(RegisterContext, "", 0);
  EXPECT_STREQ(expected3.c_str(), checked.c_str());

  utExit();
}



/* ****************************************************************************
*
* get -
*/
TEST(NotifyConditionVector, get)
{
  NotifyCondition        nc0;
  NotifyCondition        nc1;
  NotifyCondition        nc2;
  NotifyConditionVector  ncV;
  NotifyCondition*       ncP;

  utInit();

  nc0.type = "Type0";
  nc1.type = "Type1";
  nc2.type = "Type2";

  ncV.push_back(&nc0);
  ncV.push_back(&nc1);
  ncV.push_back(&nc2);

  ncP = ncV[0];
  EXPECT_STREQ("Type0", ncP->type.c_str());

  ncP = ncV[1];
  EXPECT_STREQ("Type1", ncP->type.c_str());

  ncP = ncV[2];
  EXPECT_STREQ("Type2", ncP->type.c_str());

  EXPECT_EQ(3, ncV.size());

  utExit();
}
