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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "unittest.h"

#include "parse/CompoundValueNode.h"



/* ****************************************************************************
*
* tree - 
*/
TEST(CompoundValueNode, tree)
{
  orion::CompoundValueNode*  tree  = new orion::CompoundValueNode(orion::CompoundValueNode::Object);
  orion::CompoundValueNode*  vec;
  orion::CompoundValueNode*  vecItem;
  char*                      name  = (char*) "vecItem";

  utInit();

  lmTraceLevelSet(LmtCompoundValueAdd, true);
  vec = tree->add(orion::CompoundValueNode::Vector, "vec", "");

  for (int ix = 0; ix < 5; ++ix)
  {
    vecItem = new orion::CompoundValueNode(vec, std::string("/vec/") + name, name, "a", ix, orion::CompoundValueNode::String, 2);
    vec->add(vecItem);
  }

  vecItem = vec->add(orion::CompoundValueNode::String, "vecItem6", "6");

  ASSERT_EQ(1, tree->childV.size());
  ASSERT_EQ(6, vec->childV.size());

  orion::CompoundValueNode* copy = tree->clone();

  ASSERT_EQ(1, copy->childV.size());
  ASSERT_EQ(6, copy->childV[0]->childV.size());
  ASSERT_STREQ("vecItem", copy->childV[0]->childV[0]->name.c_str());
  ASSERT_EQ(3, copy->childV[0]->childV[3]->siblingNo);
  ASSERT_EQ(2, copy->childV[0]->childV[3]->level);

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
  orion::CompoundValueNode::Type  type[]     = { orion::CompoundValueNode::Unknown, orion::CompoundValueNode::Object, orion::CompoundValueNode::Vector, orion::CompoundValueNode::String };
  const char*                     expected[] = { "Unknown",                         "Object",                         "Vector",                         "String" };

  utInit();

  for (unsigned int ix = 0; ix < sizeof(type) / sizeof(type[0]); ++ix)
    EXPECT_STREQ(expected[ix], orion::CompoundValueNode::typeName(type[ix]));
  EXPECT_STREQ("Invalid", orion::CompoundValueNode::typeName((orion::CompoundValueNode::Type) 55));

  utExit();
}



/* ****************************************************************************
*
* vectorInvalidAndOk -
*/
TEST(CompoundValueNode, vectorInvalidAndOk)
{
  lmTraceLevelSet(LmtCompoundValueAdd, true);

  orion::CompoundValueNode*  tree     = new orion::CompoundValueNode(orion::CompoundValueNode::Object);
  orion::CompoundValueNode*  vec      = new orion::CompoundValueNode(tree, "/vec", "vec", "", 0, orion::CompoundValueNode::Vector, 1);
  orion::CompoundValueNode*  item1    = new orion::CompoundValueNode(vec, std::string("/vec/vecitem1"), "vecitem1",  "a", 0, orion::CompoundValueNode::String, 2);
  const char*                outFile1 = "ngsi.compoundValue.vector.valid.xml";
  const char*                outFile2 = "ngsi.compoundValue.vector.invalid.json";

  utInit();

  tree->add(vec);
  vec->add(item1);
  vec->add(orion::CompoundValueNode::String, "vecitem", "a");

  tree->finish();
  EXPECT_STREQ("bad tag-name of vector item: /vecitem/, should be /vecitem1/", tree->error.c_str());

  item1->name = "vecitem";
  tree->finish();
  EXPECT_STREQ("OK", tree->error.c_str());

  std::string rendered;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile1)) << "Error getting test data from '" << outFile1 << "'";
  rendered = tree->render(XML, "");
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile2)) << "Error getting test data from '" << outFile2 << "'";
  rendered = tree->render(JSON, "");
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
*/
TEST(CompoundValueNode, structInvalidAndOk)
{
  lmTraceLevelSet(LmtCompoundValueAdd, true);

  orion::CompoundValueNode*  tree     = new orion::CompoundValueNode(orion::CompoundValueNode::Object);
  orion::CompoundValueNode*  str      = new orion::CompoundValueNode(tree, "/struct", "struct", "", 0, orion::CompoundValueNode::Object, 1);
  orion::CompoundValueNode*  item1    = new orion::CompoundValueNode(str, std::string("/struct/structitem"), "structitem", "a", 0, orion::CompoundValueNode::String, 2);
  orion::CompoundValueNode*  item2    = new orion::CompoundValueNode(str, std::string("/struct/structitem"), "structitem", "a", 1, orion::CompoundValueNode::String, 2);
  const char*                outFile1 = "ngsi.compoundValue.struct.valid.xml";
  const char*                outFile2 = "ngsi.compoundValue.struct.invalid.json";

  utInit();

  tree->add(str);
  str->add(item1);
  str->add(item2);

  tree->finish();
  EXPECT_STREQ("duplicated tag-name: /structitem/ in path: /struct", tree->error.c_str());

  item2->name = "structitem2";
  tree->finish();
  EXPECT_STREQ("OK", tree->error.c_str());

  std::string rendered;

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile1)) << "Error getting test data from '" << outFile1 << "'";
  rendered = tree->render(XML, "");
  EXPECT_STREQ(expectedBuf, rendered.c_str());
  
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile2)) << "Error getting test data from '" << outFile2 << "'";
  rendered = tree->render(JSON, "");
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  tree->shortShow("");
  tree->show("");

  delete tree;

  lmTraceLevelSet(LmtCompoundValueAdd, false);
  utExit();
}
