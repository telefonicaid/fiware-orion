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

#include "ngsi/ContextElementVector.h"

#include "unittest.h"



/* ****************************************************************************
*
* Check - 
*/
TEST(ContextElement, check)
{
  ContextElement ce;

  utInit();

  ce.entityId.id = "";
  EXPECT_EQ(ce.check(1, UpdateContext, "", "", 0), "empty entityId:id");

  ce.entityId.id = "id";
  EXPECT_EQ(ce.check(1, UpdateContext, "", "", 0), "OK");

  ContextAttribute a;
  a.name  = "";
  a.stringValue = "V";
  ce.contextAttributeVector.push_back(&a);
  EXPECT_EQ(ce.check(1, UpdateContext, "", "", 0), "missing attribute name");
  a.name = "name";
  
  Metadata m;
  m.name  = "";
  m.stringValue = "V";
  ce.domainMetadataVector.push_back(&m);
  EXPECT_EQ(ce.check(1, UpdateContext, "", "", 0), "missing metadata name");
  m.name = "NAME";
  EXPECT_EQ(ce.check(1, UpdateContext, "", "", 0), "OK");

  ContextElement ce2;
  ce2.entityId.id = "id";

  ContextElementVector ceVector;

  EXPECT_EQ(ceVector.check(1, UpdateContext, "", "", 0), "No context elements");

  ceVector.push_back(&ce);
  ceVector.push_back(&ce2);
  EXPECT_EQ(ceVector.check(1, UpdateContext, "", "", 0), "OK");

  // render
  const char*     outfile1 = "ngsi.contextelement.check.middle.json";
  std::string     out;

  out = ce2.render(1, false, UpdateContextElement, "", false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  // present
  ce2.present("", -1);
  ce2.present("", 1);

  m.name  = "";
  EXPECT_EQ("missing metadata name", ceVector.check(1, UpdateContext, "", "", 0));

  utExit();
}
