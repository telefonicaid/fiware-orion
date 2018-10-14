/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "unittest.h"

#include "orionTypes/OrionValueType.h"
#include "parse/CompoundValueNode.h"



/* ****************************************************************************
*
* tree -
*/
TEST(CompoundValueNode, tree)
{
  orion::CompoundValueNode*  tree  = new orion::CompoundValueNode(orion::ValueTypeObject);
  orion::CompoundValueNode*  vec;
  orion::CompoundValueNode*  vecItem;
  char*                      name  = (char*) "vecItem";

  utInit();

  lmTraceLevelSet(LmtCompoundValueAdd, true);
  vec = tree->add(orion::ValueTypeVector, "vec", "");

  for (int ix = 0; ix < 5; ++ix)
  {
    vecItem = new orion::CompoundValueNode(name, "a", orion::ValueTypeString);
    vec->add(vecItem);
  }

  vecItem = vec->add(orion::ValueTypeString, "vecItem6", "6");

  ASSERT_EQ(1, tree->childV.size());
  ASSERT_EQ(6, vec->childV.size());

  orion::CompoundValueNode* copy = tree->clone();

  ASSERT_EQ(1, copy->childV.size());
  ASSERT_EQ(6, copy->childV[0]->childV.size());
  ASSERT_STREQ("vecItem", copy->childV[0]->childV[0]->name.c_str());

  delete tree;
  delete copy;

  lmTraceLevelSet(LmtCompoundValueAdd, false);
  utExit();
}


/* ****************************************************************************
*
* typeName -
*/
TEST(CompoundValueNode, typeName)
{
  orion::ValueType  type[]     = { orion::ValueTypeNotGiven, orion::ValueTypeObject, orion::ValueTypeVector, orion::ValueTypeString };
  const char*       expected[] = { "NotGiven",               "Object",               "Vector",               "String"               };

  utInit();

  for (unsigned int ix = 0; ix < sizeof(type) / sizeof(type[0]); ++ix)
  {
    EXPECT_STREQ(expected[ix], orion::valueTypeName(type[ix]));
  }

  EXPECT_STREQ("Invalid", orion::valueTypeName((orion::ValueType) 55));

  utExit();
}



/* ****************************************************************************
*
* vectorInvalidAndOk -
*
*/
TEST(CompoundValueNode, vectorInvalidAndOk)
{
  lmTraceLevelSet(LmtCompoundValueAdd, true);

  orion::CompoundValueNode*  tree     = new orion::CompoundValueNode(orion::ValueTypeObject);
  orion::CompoundValueNode*  vec      = new orion::CompoundValueNode("vec", "", orion::ValueTypeVector);
  orion::CompoundValueNode*  item1    = new orion::CompoundValueNode("vecitem1", "a", orion::ValueTypeString);
  const char*                outFile  = "ngsi.compoundValue.vector.invalid.json";

  utInit();

  tree->add(vec);
  vec->add(item1);
  vec->add(orion::ValueTypeString, "vecitem", "a");

  std::string error = tree->finish();
  EXPECT_STREQ("bad tag-name of vector item: /vecitem/, should be /vecitem1/", error.c_str());

  item1->name = "vecitem";
  error = tree->finish();
  EXPECT_STREQ("OK", error.c_str());

  std::string rendered;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  rendered = tree->toJson();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  tree->shortShow("");
  tree->show("");

  lmTraceLevelSet(LmtCompoundValueAdd, false);

  delete tree;

  utExit();
}



/* ****************************************************************************
*
* structInvalidAndOk -
*
*/
TEST(CompoundValueNode, structInvalidAndOk)
{
  lmTraceLevelSet(LmtCompoundValueAdd, true);

  orion::CompoundValueNode*  tree     = new orion::CompoundValueNode(orion::ValueTypeObject);
  orion::CompoundValueNode*  str      = new orion::CompoundValueNode("struct", "", orion::ValueTypeObject);
  orion::CompoundValueNode*  item1    = new orion::CompoundValueNode("structitem", "a", orion::ValueTypeString);
  orion::CompoundValueNode*  item2    = new orion::CompoundValueNode("structitem", "a", orion::ValueTypeString);
  const char*                outFile  = "ngsi.compoundValue.struct.invalid.json";

  utInit();

  tree->add(str);
  str->add(item1);
  str->add(item2);

  std::string error = tree->finish();
  EXPECT_STREQ("duplicated tag-name: /structitem/ in path: /struct", error.c_str());

  item2->name = "structitem2";
  error = tree->finish();
  EXPECT_STREQ("OK", error.c_str());

  std::string rendered;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";
  rendered = tree->toJson();
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  tree->shortShow("");
  tree->show("");

  delete tree;

  lmTraceLevelSet(LmtCompoundValueAdd, false);
  utExit();
}
