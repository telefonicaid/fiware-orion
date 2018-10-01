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
  ContextAttribute*  caP = new ContextAttribute();
  std::string       res;

  utInit();

  caP->name = "";
  res     = caP->check(V1, RegisterContext);
  EXPECT_TRUE(res == "missing attribute name");

  caP->name  = "Algo, lo que sea!";
  caP->stringValue = ""; // FIXME P10: automacit value -> stringValue change, please review to check if it is safe

  res     = caP->check(V1, RegisterContext);
  EXPECT_TRUE(res == "OK");

  caP->stringValue = "Algun valor cualquiera"; // FIXME P10: automacit value -> stringValue change, please review to check if it is safe
  res     = caP->check(V1, RegisterContext);
  EXPECT_TRUE(res == "OK");

  utExit();
}



/* ****************************************************************************
*
* checkVector -
*/
TEST(ContextAttribute, checkVector)
{
  ContextAttributeVector*  caVectorP = new ContextAttributeVector();
  ContextAttribute*        ca0P      = new ContextAttribute();
  ContextAttribute*        ca1P      = new ContextAttribute();
  std::string              res;

  utInit();

  ca0P->name        = "Algo, lo que sea!";
  ca0P->stringValue = "Algo, lo que sea!"; // FIXME P10: automacit value -> stringValue change, please review to check if it is safe
  ca1P->name        = "Algo, lo que sea!";
  ca1P->stringValue = "Algo, lo que sea!"; // FIXME P10: automacit value -> stringValue change, please review to check if it is safe

  caVectorP->push_back(ca0P);
  caVectorP->push_back(ca1P);

  res     = caVectorP->check(V1, RegisterContext);
  EXPECT_TRUE(res == "OK");

  utExit();
}



/* ****************************************************************************
*
* render -
*/
TEST(ContextAttribute, render)
{
  ContextAttribute* caP = new ContextAttribute("NAME", "TYPE", "VALUE");
  std::string       out;
  const char*       outfile1 = "ngsi.contextAttribute.render.middle.json";

  utInit();

  std::vector<std::string> emptyMdV;

  out = caP->render(false, UpdateContext, emptyMdV, false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* copyMetadatas -
*/
TEST(ContextAttribute, copyMetadatas)
{
  ContextAttribute* caP = new ContextAttribute();
  Metadata*         m1P = new Metadata("m1", "int", "2");
  Metadata*         m2P = new Metadata("m2", "string", "sss");

  utInit();

  caP->metadataVector.push_back(m1P);
  caP->metadataVector.push_back(m2P);

  ContextAttribute* ca2P = new ContextAttribute(caP);
  EXPECT_EQ(2, ca2P->metadataVector.size());

  utExit();
}
