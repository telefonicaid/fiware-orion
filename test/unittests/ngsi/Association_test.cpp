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
#include "ngsi/Association.h"



/* ****************************************************************************
*
* ok - 
*/
TEST(Association, ok)
{
   Association           a;
   AttributeAssociation* aa = new AttributeAssociation();
   std::string           out;
   std::string           expected = "  <entityAssociation>\n    <sourceEntityId type=\"source\" isPattern=\"false\">\n      <id>Source</id>\n    </sourceEntityId>\n    <targetEntityId type=\"target\" isPattern=\"false\">\n      <id>Target</id>\n    </targetEntityId>\n  </entityAssociation>\n  <AttributeAssociationList>\n    <AttributeAssociation>\n      <sourceAttribute>Source</sourceAttribute>\n      <targetAttribute>Target</targetAttribute>\n    </AttributeAssociation>\n  </AttributeAssociationList>\n";

   a.entityAssociation.source.fill("Source", "source", "false");
   a.entityAssociation.target.fill("Target", "target", "false");

   aa->source = "Source";
   aa->target = "Target";

   a.attributeAssociationList.push_back(aa);

   out = a.check(RegisterContext, XML, "", "", 0);
   EXPECT_STREQ("OK", out.c_str());

   out = a.render(XML, "");
   EXPECT_STREQ(expected.c_str(), out.c_str());

   a.release();
}
