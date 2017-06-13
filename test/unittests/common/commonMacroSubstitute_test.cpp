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
#include "ngsi/ContextElement.h"
#include "ngsi/ContextAttribute.h"

#include "common/macroSubstitute.h"



/* ****************************************************************************
*
* simple - 
*/
TEST(commonMacroSubstitute, simple)
{
  ContextElement          ce("E1", "T1", "false");
  ContextAttribute*       caP = new ContextAttribute("A1", "T1", "attr1");

  ce.contextAttributeVector.push_back(caP);

  const char* s1      = "Entity ${id}/${type}, attribute '${A1}'";
  const char* correct = "Entity E1/T1, attribute 'attr1'";
  std::string result;

  macroSubstitute(&result, s1, ce);
  EXPECT_STREQ(correct, result.c_str());
}



/* ****************************************************************************
*
* withRealloc - 
*/
TEST(commonMacroSubstitute, withRealloc)
{
  ContextElement          ce("E1", "T1", "false");
  ContextAttribute*       caP = new ContextAttribute("A1", "T1", "attr1");

  ce.contextAttributeVector.push_back(caP);

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

  macroSubstitute(&result, s1, ce);
  EXPECT_STREQ(correct.c_str(), result.c_str());
}
