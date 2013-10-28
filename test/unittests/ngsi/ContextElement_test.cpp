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

#include "ngsi/ContextElementVector.h"



/* ****************************************************************************
*
* Check - 
*/
TEST(ContextElement, Check)
{
  ContextElement ce;

  ce.entityId.id = "";
  EXPECT_EQ(ce.check(UpdateContext, XML, "", "", 0), "empty entityId:id");

  ce.entityId.id = "id";
  EXPECT_EQ(ce.check(UpdateContext, XML, "", "", 0), "OK");

  ContextAttribute a;
  a.name  = "";
  a.value = "V";
  ce.contextAttributeVector.push_back(&a);
  EXPECT_EQ(ce.check(UpdateContext, XML, "", "", 0), "missing attribute name");
  a.name = "name";
  
  Metadata m;
  m.name  = "";
  m.value = "V";
  ce.domainMetadataVector.push_back(&m);
  EXPECT_EQ(ce.check(UpdateContext, XML, "", "", 0), "missing metadata name");
  m.name = "NAME";
  EXPECT_EQ(ce.check(UpdateContext, XML, "", "", 0), "OK");

  ContextElement ce2;
  ce2.entityId.id = "id";

  ContextElementVector ceVector;

  EXPECT_EQ(ceVector.check(UpdateContext, XML, "", "", 0), "No context elements");

  ceVector.push_back(&ce);
  ceVector.push_back(&ce2);
  EXPECT_EQ(ceVector.check(UpdateContext, XML, "", "", 0), "OK");

  // render
  std::string cs2render = "<contextElement>\n  <entityId type=\"\" isPattern=\"\">\n    <id>id</id>\n  </entityId>\n</contextElement>\n";
  std::string out       = ce2.render(XML, "");
  EXPECT_STREQ(cs2render.c_str(), out.c_str());

  // present
  ce2.present("", -1);
  ce2.present("", 1);

  m.name  = "";
  EXPECT_EQ(ceVector.check(UpdateContext, XML, "", "", 0), "missing metadata name");
}
