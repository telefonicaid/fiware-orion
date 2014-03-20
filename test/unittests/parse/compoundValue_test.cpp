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
#include "ngsi/ParseData.h"
#include "parse/CompoundValueNode.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/xmlRequest.h"

#include "unittest.h"



/* ****************************************************************************
*
* updateNoCompoundValue -
*/
TEST(compoundValue, updateNoCompoundValue)
{
  ParseData         reqData;
  const char*       inFile  = "ngsi10.updateContextRequest.updateNoCompoundValue.valid.xml";
  ConnectionInfo    ci("/ngsi10/updateContext", "POST", "1.1");
  ContextAttribute* caP;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);

  EXPECT_STREQ("OK", result.c_str());

  caP = reqData.upcr.res.contextElementVector.get(0)->contextAttributeVector.get(0);
  EXPECT_EQ("1", caP->value);

  utExit();
}



/* ****************************************************************************
*
* updateUnknownPath -
*/
TEST(compoundValue, updateUnknownPath)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextRequest.updateUnknownPath.invalid.xml";
  const char*     outFile = "ngsi10.updateContextResponse.updateUnknownPath.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContext", "POST", "1.1");

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);

  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* updateOneString - 
*/
TEST(compoundValue, updateOneString)
{
  ParseData                  reqData;
  const char*                inFile  = "ngsi10.updateContextRequest.updateOneString.invalid.xml";
  ConnectionInfo             ci("/ngsi10/updateContext", "POST", "1.1");
  ContextAttribute*          caP;
  orion::CompoundValueNode*  cvnRootP;
  orion::CompoundValueNode*  childP;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);

  EXPECT_STREQ("OK", result.c_str());

  caP = reqData.upcr.res.contextElementVector.get(0)->contextAttributeVector.get(0);

  EXPECT_TRUE(caP != NULL);
  EXPECT_TRUE(caP->compoundValueP != NULL);

  // Get root of compound value
  cvnRootP = caP->compoundValueP;
  
  // The root pointer of the root must be the root itself
  EXPECT_EQ(cvnRootP, cvnRootP->rootP);

  // The root should be a struct in this test case
  EXPECT_EQ(orion::CompoundValueNode::Struct, cvnRootP->type);

  // The root should have exactly one child
  EXPECT_EQ(1, cvnRootP->childV.size());

  EXPECT_EQ(0, cvnRootP->level);
  EXPECT_EQ(0, cvnRootP->siblingNo);
  EXPECT_EQ("/", cvnRootP->path);
  EXPECT_EQ("/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue", cvnRootP->root);
  
  
  // The child
  childP = cvnRootP->childV[0];

  EXPECT_EQ("s1",                            childP->name);
  EXPECT_EQ(orion::CompoundValueNode::Leaf,  childP->type);
  EXPECT_EQ("STRING",                        childP->value);
  EXPECT_EQ(0,                               childP->childV.size());

  EXPECT_EQ(cvnRootP,                        childP->container);
  EXPECT_EQ(cvnRootP,                        childP->rootP);

  EXPECT_EQ("/s1",                           childP->path);
  EXPECT_EQ(1,                               childP->level);
  EXPECT_EQ(0,                               childP->siblingNo);

  utExit();
}



/* ****************************************************************************
*
* updateTwoStrings - 
*/
TEST(compoundValue, updateTwoStrings)
{
  ParseData                  reqData;
  const char*                inFile  = "ngsi10.updateContextRequest.updateTwoStrings.invalid.xml";
  ConnectionInfo             ci("/ngsi10/updateContext", "POST", "1.1");
  ContextAttribute*          caP;
  orion::CompoundValueNode*  cvnRootP;
  orion::CompoundValueNode*  childP;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);

  EXPECT_STREQ("OK", result.c_str());

  caP = reqData.upcr.res.contextElementVector.get(0)->contextAttributeVector.get(0);
  EXPECT_TRUE(caP != NULL);
  EXPECT_TRUE(caP->compoundValueP != NULL);

  // Get root of compound value
  cvnRootP = caP->compoundValueP;
  
  // The root pointer of the root must be the root itself
  EXPECT_EQ(cvnRootP, cvnRootP->rootP);

  // The root should be a struct in this test case
  EXPECT_EQ(orion::CompoundValueNode::Struct, cvnRootP->type);

  // The root should have exactly two children
  EXPECT_EQ(2, cvnRootP->childV.size());

  EXPECT_EQ(0, cvnRootP->level);
  EXPECT_EQ(0, cvnRootP->siblingNo);
  EXPECT_EQ("/", cvnRootP->path);
  EXPECT_EQ("/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue", cvnRootP->root);

  // child 1
  childP = cvnRootP->childV[0];

  EXPECT_EQ("s1",                            childP->name);
  EXPECT_EQ(orion::CompoundValueNode::Leaf,  childP->type);
  EXPECT_EQ("STRING",                        childP->value);
  EXPECT_EQ(0,                               childP->childV.size());

  EXPECT_EQ(cvnRootP,                        childP->container);
  EXPECT_EQ(cvnRootP,                        childP->rootP);

  EXPECT_EQ("/s1",                           childP->path);
  EXPECT_EQ(1,                               childP->level);
  EXPECT_EQ(0,                               childP->siblingNo);

  // child 2
  childP = cvnRootP->childV[1];

  EXPECT_EQ("s2",                            childP->name);
  EXPECT_EQ(orion::CompoundValueNode::Leaf,  childP->type);
  EXPECT_EQ("STRING",                        childP->value);
  EXPECT_EQ(0,                               childP->childV.size());

  EXPECT_EQ(cvnRootP,                        childP->container);
  EXPECT_EQ(cvnRootP,                        childP->rootP);

  EXPECT_EQ("/s2",                           childP->path);
  EXPECT_EQ(1,                               childP->level);
  EXPECT_EQ(1,                               childP->siblingNo);

  utExit();
}



/* ****************************************************************************
*
* updateTwoItemsSameNameInStruct - 
*/
TEST(compoundValue, updateTwoItemsSameNameInStruct)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextRequest.updateTwoItemsSameNameInStruct.invalid.xml";
  const char*     outFile = "ngsi10.updateContextResponse.updateTwoItemsSameNameInStruct.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContext", "POST", "1.1");

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);

  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* updateContextValueVectorOneItem - 
*/
TEST(compoundValue, updateContextValueVectorOneItem)
{
  ParseData                  reqData;
  const char*                inFile  = "ngsi10.updateContextRequest.updateContextValueVectorOneItem.invalid.xml";
  ConnectionInfo             ci("/ngsi10/updateContext", "POST", "1.1");
  ContextAttribute*          caP;
  orion::CompoundValueNode*  cvnRootP;
  orion::CompoundValueNode*  childP;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);

  EXPECT_STREQ("OK", result.c_str());

  caP = reqData.upcr.res.contextElementVector.get(0)->contextAttributeVector.get(0);
  
  EXPECT_TRUE(caP != NULL);
  EXPECT_TRUE(caP->compoundValueP != NULL);

  // Get root of compound value
  cvnRootP = caP->compoundValueP;
  
  // The root pointer of the root must be the root itself
  EXPECT_EQ(cvnRootP, cvnRootP->rootP);

  // The root should be a 'vector' in this test case
  EXPECT_EQ(orion::CompoundValueNode::Vector, cvnRootP->type);

  // The root should have exactly one child
  EXPECT_EQ(1, cvnRootP->childV.size());

  EXPECT_EQ(0, cvnRootP->level);
  EXPECT_EQ(0, cvnRootP->siblingNo);
  EXPECT_EQ("/", cvnRootP->path);
  EXPECT_EQ("/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue", cvnRootP->root);
  
  
  // The child
  childP = cvnRootP->childV[0];

  EXPECT_EQ("vecitem",                       childP->name);
  EXPECT_EQ(orion::CompoundValueNode::Leaf,  childP->type);
  EXPECT_EQ("1",                             childP->value);
  EXPECT_EQ(0,                               childP->childV.size());

  EXPECT_EQ(cvnRootP,                        childP->container);
  EXPECT_EQ(cvnRootP,                        childP->rootP);

  EXPECT_EQ("/vecitem",                      childP->path);
  EXPECT_EQ(1,                               childP->level);
  EXPECT_EQ(0,                               childP->siblingNo);

  utExit();
}



/* ****************************************************************************
*
* updateContextValueVectorFiveItems - 
*/
TEST(compoundValue, updateContextValueVectorFiveItems)
{
  ParseData                  reqData;
  const char*                inFile  = "ngsi10.updateContextRequest.updateContextValueVectorFiveItems.invalid.xml";
  ConnectionInfo             ci("/ngsi10/updateContext", "POST", "1.1");
  ContextAttribute*          caP;
  orion::CompoundValueNode*  cvnRootP;
  orion::CompoundValueNode*  childP;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);

  EXPECT_STREQ("OK", result.c_str());

  caP = reqData.upcr.res.contextElementVector.get(0)->contextAttributeVector.get(0);
  
  EXPECT_TRUE(caP != NULL);
  EXPECT_TRUE(caP->compoundValueP != NULL);

  // Get root of compound value
  cvnRootP = caP->compoundValueP;
  
  // The root pointer of the root must be the root itself
  EXPECT_EQ(cvnRootP, cvnRootP->rootP);

  // The root should be a 'vector' in this test case
  EXPECT_EQ(orion::CompoundValueNode::Vector, cvnRootP->type);

  // The root should have five children
  EXPECT_EQ(5, cvnRootP->childV.size());

  EXPECT_EQ(0, cvnRootP->level);
  EXPECT_EQ(0, cvnRootP->siblingNo);
  EXPECT_EQ("/", cvnRootP->path);
  EXPECT_EQ("/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue", cvnRootP->root);
  
  
  // Child 1-5
  std::string value[] = { "1", "2", "3", "4", "5" };
  for (unsigned int childIx = 0; childIx < 5; ++childIx)
  {
    childP = cvnRootP->childV[childIx];

    EXPECT_EQ("vecitem",                       childP->name);
    EXPECT_EQ(orion::CompoundValueNode::Leaf,  childP->type);
    EXPECT_EQ(value[childIx],                  childP->value);
    EXPECT_EQ(0,                               childP->childV.size());

    EXPECT_EQ(cvnRootP,                        childP->container);
    EXPECT_EQ(cvnRootP,                        childP->rootP);

    EXPECT_EQ("/vecitem",                      childP->path);
    EXPECT_EQ(1,                               childP->level);
    EXPECT_EQ(childIx,                         childP->siblingNo);
  }

  utExit();
}




/* ****************************************************************************
*
* updateContextValueVectorFiveItemsPlusBadOne - 
*/
TEST(compoundValue, updateContextValueVectorFiveItemsPlusBadOne)
{
  ParseData       reqData;
  const char*     inFile  = "ngsi10.updateContextRequest.updateContextValueVectorFiveItemsPlusBadOne.invalid.xml";
  const char*     outFile = "ngsi10.updateContextResponse.updateContextValueVectorFiveItemsPlusBadOne.valid.xml";
  ConnectionInfo  ci("/ngsi10/updateContext", "POST", "1.1");

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outFile)) << "Error getting test data from '" << outFile << "'";

  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);

  EXPECT_STREQ(expectedBuf, result.c_str());

  utExit();
}



/* ****************************************************************************
*
* updateTwoStructs - 
*/
TEST(compoundValue, updateTwoStructs)
{
  ParseData                  reqData;
  const char*                inFile        = "ngsi10.updateContextRequest.updateTwoStructs.invalid.xml";
  const char*                renderedFile  = "ngsi.contextAttribute.updateTwoStructsRendered.invalid.xml";
  ConnectionInfo             ci("/ngsi10/updateContext", "POST", "1.1");
  ContextAttribute*          caP;
  std::string                rendered;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), inFile)) << "Error getting test data from '" << inFile << "'";
  std::string result = xmlTreat(testBuf, &ci, &reqData, UpdateContext, "updateContextRequest", NULL);
  EXPECT_STREQ("OK", result.c_str());

  caP = reqData.upcr.res.contextElementVector.get(0)->contextAttributeVector.get(0);
  

  EXPECT_TRUE(caP != NULL);
  EXPECT_TRUE(caP->compoundValueP != NULL);

  rendered = caP->render(XML, "");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), renderedFile)) << "Error getting test data from '" << renderedFile << "'";
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  orion::CompoundValueNode*  cvnRootP;
  orion::CompoundValueNode*  structP;
  orion::CompoundValueNode*  childP;

  // Get root of compound value
  cvnRootP = caP->compoundValueP;
  
  // The root pointer of the root must be the root itself
  EXPECT_EQ(cvnRootP, cvnRootP->rootP);

  // The root should be a 'struct' in this test case
  EXPECT_EQ(orion::CompoundValueNode::Struct, cvnRootP->type);

  // The root should have two children
  EXPECT_EQ(2, cvnRootP->childV.size());

  EXPECT_EQ(0, cvnRootP->level);
  EXPECT_EQ(0, cvnRootP->siblingNo);
  EXPECT_EQ("/", cvnRootP->path);
  EXPECT_EQ("/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue", cvnRootP->root);
  

  // Now, child struct 1
  structP = cvnRootP->childV[0];

  EXPECT_EQ("struct1",                         structP->name);
  EXPECT_EQ(orion::CompoundValueNode::Struct,  structP->type);
  EXPECT_EQ(2,                                 structP->childV.size());

  EXPECT_EQ(cvnRootP,                          structP->container);
  EXPECT_EQ(cvnRootP,                          structP->rootP);

  EXPECT_EQ("/struct1",                        structP->path);
  EXPECT_EQ(1,                                 structP->level);
  EXPECT_EQ(0,                                 structP->siblingNo);

  printf("structP->childV.size == %lu\n", structP->childV.size());

  // Child 1 of struct1
  childP = structP->childV[0];

  EXPECT_EQ("s1-1",                            childP->name);
  EXPECT_EQ(orion::CompoundValueNode::Leaf,    childP->type);
  EXPECT_EQ("1-1",                             childP->value);
  EXPECT_EQ(0,                                 childP->childV.size());

  EXPECT_EQ(structP,                           childP->container);
  EXPECT_EQ(cvnRootP,                          childP->rootP);

  EXPECT_EQ("/struct1/s1-1",                   childP->path);
  EXPECT_EQ(2,                                 childP->level);
  EXPECT_EQ(0,                                 childP->siblingNo);


  // Child 2 of struct1
  childP = cvnRootP->childV[0]->childV[1];

  EXPECT_EQ("s1-2",                            childP->name);
  EXPECT_EQ(orion::CompoundValueNode::Leaf,    childP->type);
  EXPECT_EQ("1-2",                             childP->value);
  EXPECT_EQ(0,                                 childP->childV.size());

  EXPECT_EQ(structP,                           childP->container);
  EXPECT_EQ(cvnRootP,                          childP->rootP);

  EXPECT_EQ("/struct1/s1-2",                   childP->path);
  EXPECT_EQ(2,                                 childP->level);
  EXPECT_EQ(1,                                 childP->siblingNo);



  // child struct 2
  structP = cvnRootP->childV[1];

  EXPECT_EQ("struct2",                         structP->name);
  EXPECT_EQ(orion::CompoundValueNode::Struct,  structP->type);
  EXPECT_EQ(2,                                 structP->childV.size());

  EXPECT_EQ(cvnRootP,                          structP->container);
  EXPECT_EQ(cvnRootP,                          structP->rootP);

  EXPECT_EQ("/struct2",                        structP->path);
  EXPECT_EQ(1,                                 structP->level);
  EXPECT_EQ(1,                                 structP->siblingNo);

  // Child 1 of struct2
  childP = cvnRootP->childV[1]->childV[0];

  EXPECT_EQ("s2-1",                            childP->name);
  EXPECT_EQ(orion::CompoundValueNode::Leaf,    childP->type);
  EXPECT_EQ("2-1",                             childP->value);
  EXPECT_EQ(0,                                 childP->childV.size());

  EXPECT_EQ(structP,                           childP->container);
  EXPECT_EQ(cvnRootP,                          childP->rootP);

  EXPECT_EQ("/struct2/s2-1",                   childP->path);
  EXPECT_EQ(2,                                 childP->level);
  EXPECT_EQ(0,                                 childP->siblingNo);


  // Child 2 of struct2
  childP = cvnRootP->childV[1]->childV[1];

  EXPECT_EQ("s2-2",                            childP->name);
  EXPECT_EQ(orion::CompoundValueNode::Leaf,    childP->type);
  EXPECT_EQ("2-2",                             childP->value);
  EXPECT_EQ(0,                                 childP->childV.size());

  EXPECT_EQ(structP,                           childP->container);
  EXPECT_EQ(cvnRootP,                          childP->rootP);

  EXPECT_EQ("/struct2/s2-2",                   childP->path);
  EXPECT_EQ(2,                                 childP->level);
  EXPECT_EQ(1,                                 childP->siblingNo);

  utExit();
}
