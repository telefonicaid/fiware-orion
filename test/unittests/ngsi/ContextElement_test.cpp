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
  ContextElement* ceP = new ContextElement();

  utInit();

  ceP->entityId.id = "";
  EXPECT_EQ(ceP->check(V1, UpdateContext), "empty entityId:id");

  ceP->entityId.id = "id";
  EXPECT_EQ(ceP->check(V1, UpdateContext), "OK");

  ContextAttribute* aP = new ContextAttribute();
  aP->name  = "";
  aP->stringValue = "V";
  ceP->contextAttributeVector.push_back(aP);
  EXPECT_EQ(ceP->check(V1, UpdateContext), "missing attribute name");
  aP->name = "name";

  Metadata* mP = new Metadata();
  mP->name  = "";
  mP->stringValue = "V";
  ceP->domainMetadataVector.push_back(mP);
  EXPECT_EQ(ceP->check(V1, UpdateContext), "missing metadata name");
  mP->name = "NAME";
  EXPECT_EQ(ceP->check(V1, UpdateContext), "OK");

  ContextElement* ce2P = new ContextElement();
  ce2P->entityId.id = "id";

  ContextElementVector* ceVectorP = new ContextElementVector();

  EXPECT_EQ(ceVectorP->check(V1, UpdateContext), "No context elements");

  ceVectorP->push_back(ceP);
  ceVectorP->push_back(ce2P);
  EXPECT_EQ(ceVectorP->check(V1, UpdateContext), "OK");

  // render
  const char*     outfile1 = "ngsi.contextelement.check.middle.json";
  std::string     out;

  out = ce2P->render(V1, false, UpdateContextElement, false, false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  // present
  ce2P->present("", -1);
  ce2P->present("", 1);

  mP->name  = "";
  EXPECT_EQ("missing metadata name", ceVectorP->check(V1, UpdateContext));

  utExit();
}
