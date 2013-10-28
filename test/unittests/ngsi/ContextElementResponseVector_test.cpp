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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ContextElementResponseVector.h"



/* ****************************************************************************
*
* check - 
*/
TEST(ContextElementResponseVector, check)
{
  ContextElementResponseVector  cerv;
  ContextElementResponse        cer;
  std::string                   out;

  out = cerv.check(UpdateContext, XML, "", "", 0);
  EXPECT_STREQ("OK", out.c_str());

  cer.contextElement.entityId.id         = "ID";
  cer.contextElement.entityId.type       = "Type";
  cer.contextElement.entityId.isPattern  = "false";
  cer.statusCode.fill(SccOk, "reason", "details");

  cerv.push_back(&cer);
  out = cerv.check(UpdateContext, XML, "", "", 0);
  EXPECT_STREQ("OK", out.c_str());
}



/* ****************************************************************************
*
* render - 
*/
TEST(ContextElementResponseVector, render)
{
  ContextElementResponseVector  cerv;
  ContextElementResponse        cer;
  std::string                   out;
  std::string                   expected = "<contextResponseList>\n  <contextElementResponse>\n    <contextElement>\n      <entityId type=\"Type\" isPattern=\"false\">\n        <id>ID</id>\n      </entityId>\n    </contextElement>\n    <statusCode>\n      <code>200</code>\n      <reasonPhrase>reason</reasonPhrase>\n      <details>details</details>\n    </statusCode>\n  </contextElementResponse>\n</contextResponseList>\n";

  out = cerv.render(XML, "");
  EXPECT_STREQ("", out.c_str());

  cer.contextElement.entityId.id         = "ID";
  cer.contextElement.entityId.type       = "Type";
  cer.contextElement.entityId.isPattern  = "false";
  cer.statusCode.fill(SccOk, "reason", "details");

  cerv.push_back(&cer);
  out = cerv.render(XML, "");
  EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* present -
*
* Just to exercise the code, nothing to be expected here ...
*/
TEST(ContextElementResponseVector, present)
{
   ContextElementResponseVector  cerv;
   ContextElementResponse        cer;

   cer.contextElement.entityId.id         = "ID";
   cer.contextElement.entityId.type       = "Type";
   cer.contextElement.entityId.isPattern  = "false";
   cer.statusCode.fill(SccOk, "reason", "details");
   cerv.push_back(&cer);

   cerv.present("");
}

