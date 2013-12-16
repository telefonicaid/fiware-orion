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

#include "ngsi/ContextElementResponse.h"



/* ****************************************************************************
*
* check - 
*/
TEST(ContextElementResponse, check)
{
   ContextElementResponse  cer;
   std::string             out;
   
   out = cer.check(UpdateContext, XML, "", "", 0);
   EXPECT_STREQ("empty entityId:id", out.c_str());

   cer.contextElement.entityId.id         = "ID";
   cer.contextElement.entityId.type       = "Type";
   cer.contextElement.entityId.isPattern  = "false";

   out = cer.check(UpdateContext, XML, "", "", 0);
   EXPECT_STREQ("no code", out.c_str());

   cer.statusCode.fill(SccOk, "reason", "details");
   out = cer.check(UpdateContext, XML, "", "", 0);
   EXPECT_STREQ("OK", out.c_str());
}



/* ****************************************************************************
*
* render - 
*/
TEST(ContextElementResponse, render)
{
   ContextElementResponse  cer;
   std::string             expected1xml  = "<contextElementResponse>\n  <contextElement>\n    <entityId type=\"Type\" isPattern=\"false\">\n      <id>ID</id>\n    </entityId>\n  </contextElement>\n  <statusCode>\n    <code>200</code>\n    <reasonPhrase>reason</reasonPhrase>\n    <details>details</details>\n  </statusCode>\n</contextElementResponse>\n";
   std::string             expected1json = "{\n  \"contextElement\" : {\n    \"type\" : \"Type\",\n    \"isPattern\" : \"false\",\n    \"id\" : \"ID\"\n  },\n  \"statusCode\" : {\n    \"code\" : \"200\",\n    \"reasonPhrase\" : \"reason\",\n    \"details\" : \"details\"\n  }\n}\n";
   std::string             out;

   cer.contextElement.entityId.id         = "ID";
   cer.contextElement.entityId.type       = "Type";
   cer.contextElement.entityId.isPattern  = "false";

   cer.statusCode.fill(SccOk, "reason", "details");

   out = cer.render(XML, "");
   EXPECT_STREQ(expected1xml.c_str(), out.c_str());
   out = cer.render(JSON, "");
   EXPECT_STREQ(expected1json.c_str(), out.c_str());
}


/* ****************************************************************************
*
* present - 
*/
TEST(ContextElementResponse, present)
{
   ContextElementResponse cer;

   cer.contextElement.entityId.id         = "ID";
   cer.contextElement.entityId.type       = "Type";
   cer.contextElement.entityId.isPattern  = "false";

   cer.statusCode.fill(SccOk, "reason", "details");

   cer.present("", 0);
}
