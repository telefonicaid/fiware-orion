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
#include "ngsi/MetadataVector.h"

#include "unittest.h"



/* ****************************************************************************
*
* render -
*/
TEST(MetadataVector, render)
{
  Metadata        m("Name", "Type", "Value");
  Metadata        m2("Name2", "Type2", "Value2");
  MetadataVector  mV;
  const char*     outfile1 = "ngsi.metadataVector.render1.middle.json";
  const char*     outfile2 = "ngsi.metadataVector.render3.middle.json";
  std::string     out;

  utInit();

  mV.push_back(&m);

  std::vector<Metadata*>  metadataFilter;

  metadataFilter.push_back(&m);

  out = mV.toJsonV1(metadataFilter, false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  metadataFilter.push_back(&m2);

  mV.push_back(&m2); 
  out = mV.toJsonV1(metadataFilter, false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
