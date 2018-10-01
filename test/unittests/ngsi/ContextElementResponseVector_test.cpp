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

#include "ngsi/ContextElementResponseVector.h"

#include "unittest.h"



/* ****************************************************************************
*
* check - 
*/
TEST(ContextElementResponseVector, check)
{
  ContextElementResponseVector  cerv;
  ContextElementResponse        cer;
  std::string                   out;

  utInit();

  out = cerv.check(V1, UpdateContext, "", 0);
  EXPECT_STREQ("OK", out.c_str());

  cer.entity.id         = "ID";
  cer.entity.type       = "Type";
  cer.entity.isPattern  = "false";
  cer.statusCode.fill(SccOk, "details");

  cerv.push_back(&cer);
  out = cerv.check(V1, UpdateContext, "", 0);
  EXPECT_STREQ("OK", out.c_str());

  utExit();
}



/* ****************************************************************************
*
* render - 
*
*/
TEST(ContextElementResponseVector, render)
{
  ContextElementResponseVector  cerv;
  ContextElementResponse        cer;
  std::string                   out;

  utInit();

  std::vector<std::string> emptyV;

  // FIXME P2: "" is string, function signature says bool..
  out = cerv.render(false, UpdateContextElement, emptyV, false, emptyV, "");
  EXPECT_STREQ("", out.c_str());

  cer.entity.id         = "ID";
  cer.entity.type       = "Type";
  cer.entity.isPattern  = "false";
  cer.statusCode.fill(SccOk, "details");

  utExit();
}
