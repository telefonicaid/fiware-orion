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
#include "ngsi/Association.h"

#include "unittest.h"



/* ****************************************************************************
*
* ok - 
*/
TEST(AttributeAssociationList, ok)
{
  AttributeAssociationList aal;
  AttributeAssociation     aa;
  std::string              out;
  const char*              outfile = "ngsi.attributeAssociationList.ok.valid.xml";

  utInit();

  out = aal.render(XML, "", false);
  EXPECT_STREQ("", out.c_str());

  aa.source = "Source";
  aa.target = "Target";

  aal.push_back(&aa);
  ASSERT_EQ(1, aal.size());

  out = aal.render(XML, "", false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  AttributeAssociation* aa1;

  aa1 = aal.get(0);
  EXPECT_TRUE(aa1 != NULL);
  EXPECT_STREQ(aa1->source.c_str(), "Source");

  // Just to exercise the code ...
  aal.present("");

  utExit();
}



/* ****************************************************************************
*
* invalidAttributeAssociation - 
*/
TEST(AttributeAssociationList, invalidAttributeAssociation)
{
  AttributeAssociationList aal;
  AttributeAssociation     aa;
  std::string              out;
  std::string              expected1 = "empty target";
  std::string              expected2 = "empty source";

  aa.source = "S";
  aa.target = "";

  aal.push_back(&aa);
  out = aal.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ(expected1, out);

  aa.source = "";
  aa.target = "T";
  out = aal.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ(expected2, out);
}
