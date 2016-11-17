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

#include "ngsi/ContextAttributeVector.h"

#include "unittest.h"



/* ****************************************************************************
*
* checkOne - 
*/
TEST(ContextAttribute, checkOne)
{
  ContextAttribute  ca;
  std::string       res;

  utInit();

  ca.name = "";
  res     = ca.check("v1", RegisterContext);
  EXPECT_TRUE(res == "missing attribute name");

  ca.name  = "Algo, lo que sea!";
  ca.stringValue = ""; // FIXME P10: automacit value -> stringValue change, please review to check if it is safe

  res     = ca.check("v1", RegisterContext);
  EXPECT_TRUE(res == "OK");
  
  ca.stringValue = "Algun valor cualquiera"; // FIXME P10: automacit value -> stringValue change, please review to check if it is safe
  res     = ca.check("v1", RegisterContext);
  EXPECT_TRUE(res == "OK");

  utExit();
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

  utInit();

  ca0.name        = "Algo, lo que sea!";
  ca0.stringValue = "Algo, lo que sea!"; // FIXME P10: automacit value -> stringValue change, please review to check if it is safe
  ca1.name        = "Algo, lo que sea!";
  ca1.stringValue = "Algo, lo que sea!"; // FIXME P10: automacit value -> stringValue change, please review to check if it is safe

  caVector.push_back(&ca0);
  caVector.push_back(&ca1);

  res     = caVector.check("v1", RegisterContext);
  EXPECT_TRUE(res == "OK");

  utExit();
}



/* ****************************************************************************
*
* render - 
*/
TEST(ContextAttribute, render)
{
  ContextAttribute  ca("NAME", "TYPE", "VALUE");
  std::string       out;
  const char*       outfile1 = "ngsi.contextAttribute.render.middle.json";

  utInit();

  out = ca.render("v1", false, UpdateContext, "");
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* present - 
*/
TEST(ContextAttribute, present)
{
  // Just to exercise all the code ..
  ContextAttribute  ca("NAME", "TYPE", "VALUE");

  utInit();

  ca.present("", 0);

  utExit();
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

  utInit();

  ca.metadataVector.push_back(&m1);
  ca.metadataVector.push_back(&m2);

  ContextAttribute ca2(&ca);
  EXPECT_EQ(2, ca2.metadataVector.size());

  utExit();
}
