/*
*
* Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "ngsi/ContextAttribute.h"

#include "common/macroSubstitute.h"



/* ****************************************************************************
*
* simple - 
*/
TEST(commonMacroSubstitute, simple)
{
  Entity             en("E1", "T1", "false");
  ContextAttribute*  caP = new ContextAttribute("A1", "T1", "attr1");
  bool               b;

  en.attributeVector.push_back(caP);

  const char* s1      = "Entity ${id}/${type}, attribute '${A1}'";
  const char* correct = "Entity E1/T1, attribute 'attr1'";
  std::string result;

  b = macroSubstitute(&result, s1, en);
  EXPECT_TRUE(b);
  EXPECT_STREQ(correct, result.c_str());
}



/* ****************************************************************************
*
* withRealloc - 
*/
TEST(commonMacroSubstitute, withRealloc)
{
  Entity             en("E1", "T1", "false");
  ContextAttribute*  caP = new ContextAttribute("A1", "T1", "attr1");
  bool               b;

  en.attributeVector.push_back(caP);

  const char* base = 
    "The test string needs to be longer than 1024 chars to test reallocs in Macro substitution"
    "So, this line is just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ..."
    " ---------------------------------------------------------------------------- this line is also just filling, and ...";
  
  std::string s1      = std::string(base) + "Now, finally something to substitute: Entity ${id}/${type}, attribute '${A1}'";
  std::string correct = std::string(base) + "Now, finally something to substitute: Entity E1/T1, attribute 'attr1'";
  std::string result;

  b = macroSubstitute(&result, s1, en);
  EXPECT_TRUE(b);
  EXPECT_STREQ(correct.c_str(), result.c_str());
}



/* ****************************************************************************
*
* bufferTooBigInitially - max size of the substituted buffer is 8MB (outReqMsgMaxSize)
*
* This unit test provokes a buffer size > 8MB to see the error returned
*/
TEST(commonMacroSubstitute, bufferTooBigInitially)
{
  bool               b;
  Entity             en("EntityId000001", "EntityType000001", "false");
  ContextAttribute*  caP = new ContextAttribute("A1", "T1", "attr1");

  en.attributeVector.push_back(caP);

  char* base = (char*) malloc(outReqMsgMaxSize + 2);

  memset(base, 'a', outReqMsgMaxSize + 1);
  base[outReqMsgMaxSize + 1] = 0;

  std::string s1      = std::string(base) + "${id}/${type}";
  // correct          = std::string(base) + "EntityId000001/EntityType000001";
  std::string result;

  b = macroSubstitute(&result, s1, en);
  EXPECT_FALSE(b);
  EXPECT_STREQ("", result.c_str());

  free(base);
}



/* ****************************************************************************
*
* bufferTooBigAfterSubstitution - max size of the substituted buffer is 8MB (outReqMsgMaxSize)
*
* This unit test provokes a buffer size > 8MB to see the error returned.
* However, unlike 'bufferTooBigInitially', this test has an incoming buffer < 8MB but
* as the buffer grows as substitutions are made, the resulting buffer is > 8MB and an
* error should be returned
*/
TEST(commonMacroSubstitute, bufferTooBigAfterSubstitution)
{
  bool               b;
  Entity             en("EntityId000001", "EntityType000001", "false");
  ContextAttribute*  caP = new ContextAttribute("A1", "T1", "attr1");

  en.attributeVector.push_back(caP);

  char* base = (char*) malloc(outReqMsgMaxSize + 2 - 16);  // -16 so that '${id}/${type}' fits inside 8MB
                                                           // but 'EntityId000001/EntityType000001' does not

  memset(base, 'a', outReqMsgMaxSize + 2 - 16);
  base[outReqMsgMaxSize + 2 - 16] = 0;

  std::string s1      = std::string(base) + "${id}/${type}";                   // < 8MB
  //          correct = std::string(base) + "EntityId000001/EntityType000001"; // > 8MB after substitutions
  std::string result;

  b = macroSubstitute(&result, s1, en);
  EXPECT_FALSE(b);
  EXPECT_STREQ("", result.c_str());

  free(base);
}
