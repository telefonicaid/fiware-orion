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

#include "ngsi/ContextElementResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* check -
*/
TEST(ContextElementResponse, check)
{
   ContextElementResponse  cer;
   std::string             out;

   utInit();

   out = cer.check(V1, UpdateContext, "", 0);
   EXPECT_STREQ("empty entityId:id", out.c_str());

   cer.contextElement.entityId.id         = "ID";
   cer.contextElement.entityId.type       = "Type";
   cer.contextElement.entityId.isPattern  = "false";

   out = cer.check(V1, UpdateContext, "", 0);
   EXPECT_STREQ("no code", out.c_str());

   cer.statusCode.fill(SccOk, "details");
   out = cer.check(V1, UpdateContext, "", 0);
   EXPECT_STREQ("OK", out.c_str());

   utExit();
}



/* ****************************************************************************
*
* render -
*/
TEST(ContextElementResponse, render)
{
  ContextElementResponse  cer;
  const char*             outfile = "ngsi.contextElementResponse.render.middle.json";
  std::string             out;

   utInit();

   cer.contextElement.entityId.id         = "ID";
   cer.contextElement.entityId.type       = "Type";
   cer.contextElement.entityId.isPattern  = "false";

   cer.statusCode.fill(SccOk, "details");

   out = cer.render(V1, false, UpdateContextElement, false);
   EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
   EXPECT_STREQ(expectedBuf, out.c_str());

   utExit();
}
