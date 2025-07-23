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

#include "apiTypesV2/EntityVector.h"



/* ****************************************************************************
*
* check
*/
TEST(Entity, check)
{
  utInit();

  Entity* enP         = new Entity();
  enP->entityId.id    = "E";
  enP->entityId.type  = "T";

  ContextAttribute* caP = new ContextAttribute("A", "T", "val");
  enP->attributeVector.push_back(caP);

  EXPECT_EQ("OK", enP->check(EntitiesRequest));

  enP->entityId.id = "";
  EXPECT_EQ("id and idPattern cannot be both empty at the same time", enP->check(EntitiesRequest));

  enP->entityId.id = "E<1>";
  EXPECT_EQ(ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID, enP->check(EntitiesRequest));

  enP->entityId.idPattern = "E<1>";
  enP->entityId.id        = "";
  EXPECT_EQ("OK", enP->check(EntitiesRequest));

  enP->entityId.id        = "E";
  enP->entityId.idPattern = "";

  enP->entityId.type = "T<1>";
  EXPECT_EQ(ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE, enP->check(EntitiesRequest));

  enP->entityId.typePattern = "T<1>";
  enP->entityId.type        = "";
  EXPECT_EQ("OK", enP->check(EntitiesRequest));

  delete enP;

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
  utInit();

  Entity* enP = new Entity();

  enP->entityId.id = "";
  EXPECT_EQ(enP->check(BatchUpdateRequest), "id and idPattern cannot be both empty at the same time");

  enP->entityId.id = "id";
  EXPECT_EQ(enP->check(BatchUpdateRequest), "type and typePattern cannot be both empty at the same time");

  ContextAttribute* aP = new ContextAttribute();
  aP->name  = "";
  aP->stringValue = "V";
  enP->attributeVector.push_back(aP);
  EXPECT_EQ(enP->check(BatchUpdateRequest), "type and typePattern cannot be both empty at the same time");
  aP->name = "name";

  Entity* en2P = new Entity("id", "", "", "");
  en2P->renderId = true;

  EntityVector* ceVectorP = new EntityVector();

  ceVectorP->push_back(enP);
  ceVectorP->push_back(en2P);

  // render
  const char*               outfile1 = "ngsi.contextelement.check.middle.json";
  std::string               out;

  out = en2P->toJson(NGSI_V2_NORMALIZED, false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  delete ceVectorP;

  utExit();
}
