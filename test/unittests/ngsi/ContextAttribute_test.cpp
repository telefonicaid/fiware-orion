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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ContextAttributeVector.h"



/* ****************************************************************************
*
* checkOne - 
*/
TEST(ContextAttribute, checkOne)
{
  ContextAttribute  ca;
  std::string       res;

  ca.name = "";
  res     = ca.check(RegisterContext, XML, "", "", 0);
  EXPECT_TRUE(res == "missing attribute name");

  ca.name  = "Algo, lo que sea!";
  ca.value = "";

  res     = ca.check(RegisterContext, XML, "", "", 0);
  EXPECT_TRUE(res == "missing attribute value");
  
  ca.value = "Algun valor cualquiera";
  res     = ca.check(RegisterContext, XML, "", "", 0);
  EXPECT_TRUE(res == "OK");
}



/* ****************************************************************************
*
* checkVector - 
*/
TEST(ContextAttribute, checkVector)
{
  ContextAttributeVector  caVector;
  ContextAttribute        ca0;
  ContextAttribute        ca1;
  std::string             res;

  ca0.name  = "Algo, lo que sea!";
  ca0.value = "Algo, lo que sea!";
  ca1.name  = "Algo, lo que sea!";
  ca1.value = "Algo, lo que sea!";

  caVector.push_back(&ca0);
  caVector.push_back(&ca1);

  res     = caVector.check(RegisterContext, XML, "", "", 0);
  EXPECT_TRUE(res == "OK");
}



/* ****************************************************************************
*
* render - 
*/
TEST(ContextAttribute, render)
{
  ContextAttribute  ca("NAME", "TYPE", "VALUE");
  std::string       out;
  std::string       xml  = "<contextAttribute>\n  <name>NAME</name>\n  <type>TYPE</type>\n  <contextValue>VALUE</contextValue>\n</contextAttribute>\n";
  std::string       json = "{\n  \"name\" : \"NAME\",\n  \"type\" : \"TYPE\",\n  \"contextValue\" : \"VALUE\"\n}\n";

  out = ca.render(XML, "");
  EXPECT_STREQ(xml.c_str(), out.c_str());

  out = ca.render(JSON, "");
  EXPECT_STREQ(json.c_str(), out.c_str());
}



/* ****************************************************************************
*
* present - 
*/
TEST(ContextAttribute, present)
{
  // Just to exercise all the code ..
  ContextAttribute  ca("NAME", "TYPE", "VALUE");

  ca.present("", 0);
}



/* ****************************************************************************
*
* copyMetadatas - 
*/
TEST(ContextAttribute, copyMetadatas)
{
  ContextAttribute  ca;
  Metadata          m1("m1", "int", "2");
  Metadata          m2("m2", "string", "sss");

  ca.metadataVector.push_back(&m1);
  ca.metadataVector.push_back(&m2);

  ContextAttribute ca2(&ca);
  EXPECT_EQ(2, ca2.metadataVector.size());
}
