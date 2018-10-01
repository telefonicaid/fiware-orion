/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*/

#include <string>
#include <vector>

#include "apiTypesV2/Entity.h"
#include "common/errorMessages.h"
#include "unittests/unittest.h"

#include "ngsi/ContextElementVector.h"



/* ****************************************************************************
*
* check
*/
TEST(Entity, check)
{
  utInit();

  Entity* enP         = new Entity();
  enP->id             = "E";
  enP->type           = "T";
  enP->isPattern      = "false";
  enP->isTypePattern  = false;

  ContextAttribute* caP = new ContextAttribute("A", "T", "val");
  enP->attributeVector.push_back(caP);

  EXPECT_EQ("OK", enP->check(V2, EntitiesRequest));

  enP->id = "";
  EXPECT_EQ("entity id length: 0, min length supported: 1", enP->check(V2, EntitiesRequest));

  enP->id = "E<1>";
  EXPECT_EQ(ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID, enP->check(V2, EntitiesRequest));
  enP->isPattern = "true";
  EXPECT_EQ("OK", enP->check(V2, EntitiesRequest));
  enP->id        = "E";
  enP->isPattern = "false";

  enP->type = "T<1>";
  EXPECT_EQ(ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE, enP->check(V2, EntitiesRequest));
  enP->isTypePattern  = true;
  EXPECT_EQ("OK", enP->check(V2, EntitiesRequest));
  enP->type = "T";

  enP->isPattern = "<false>";
  EXPECT_EQ("Invalid value for isPattern", enP->check(V2, EntitiesRequest));

  utExit();
}


/* ****************************************************************************
*
* Check -
*
* Test ported from old Context Element class test
*/
TEST(Entity, checkV1)
{
  Entity* enP = new Entity();

  utInit();

  enP->id = "";
  EXPECT_EQ(enP->check(V1, UpdateContext), "empty entityId:id");

  enP->id = "id";
  EXPECT_EQ(enP->check(V1, UpdateContext), "OK");

  ContextAttribute* aP = new ContextAttribute();
  aP->name  = "";
  aP->stringValue = "V";
  enP->attributeVector.push_back(aP);
  EXPECT_EQ(enP->check(V1, UpdateContext), "missing attribute name");
  aP->name = "name";

  Entity* en2P = new Entity("id", "", "false");

  ContextElementVector* ceVectorP = new ContextElementVector();

  EXPECT_EQ(ceVectorP->check(V1, UpdateContext), "No context elements");

  ceVectorP->push_back(enP);
  ceVectorP->push_back(en2P);
  EXPECT_EQ(ceVectorP->check(V1, UpdateContext), "OK");

  // render
  const char*               outfile1 = "ngsi.contextelement.check.middle.json";
  std::string               out;
  std::vector<std::string>  emptyV;

  out = en2P->toJsonV1(false, UpdateContextElement, emptyV, false, emptyV, false, false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", ceVectorP->check(V1, UpdateContext));

  utExit();
}
