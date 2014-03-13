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

#include "unittest.h"

#include "parse/CompoundValueNode.h"



/* ****************************************************************************
*
* tree - 
*/
TEST(CompoundValueNode, tree)
{
  orion::CompoundValueNode*  tree  = new orion::CompoundValueNode("XXX");
  orion::CompoundValueNode*  vec   = new orion::CompoundValueNode(tree, "/vec", "vec", "", 0, orion::CompoundValueNode::Vector, 1);
  orion::CompoundValueNode*  vecItem;
  char*                      name  = (char*) "vecItem";

  utInit();

  tree->add(vec);

  for (int ix = 0; ix < 5; ++ix)
  {
    vecItem = new orion::CompoundValueNode(vec, std::string("/vec/") + name, name, "a", ix, orion::CompoundValueNode::Leaf, 2);
    vec->add(vecItem);
  }

  vecItem = vec->add(orion::CompoundValueNode::Leaf, "vecItem6", "/vec", "6");

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

  utExit();
}


/* ****************************************************************************
*
* typeName - 
*/
TEST(CompoundValueNode, typeName)
{
  orion::CompoundValueNode        cvn("xxx");
  orion::CompoundValueNode::Type  type[]     = { orion::CompoundValueNode::Struct, orion::CompoundValueNode::Vector, orion::CompoundValueNode::Leaf };
  const char*                     expected[] = { "Struct",                         "Vector",                         "Leaf" };

  for (unsigned int ix = 0; ix < sizeof(type) / sizeof(type[0]); ++ix)
  {
     EXPECT_STREQ(expected[ix], cvn.typeName(type[ix]));
  }
}

