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

#include "ngsi/AttributeAssociation.h"

#include "unittest.h"



/* ****************************************************************************
*
* ok - 
*/
TEST(AttributeAssociation, ok)
{
  AttributeAssociation  aa;
  std::string           out;
  const char*           outfile = "ngsi.attributeAssociation.ok.middle.xml";

  utInit();

  aa.source = "source";
  aa.target = "target";

  out = aa.render(XML, "  ", false);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  out = aa.check(RegisterContext, XML, "", "", 0);
  EXPECT_EQ("OK", out);

  // Just to exercise the code
  aa.present("", -1);
  aa.present("", 5);

  utExit();
}



/* ****************************************************************************
*
* not_ok - 
*/
TEST(AttributeAssociation, not_ok)
{
   AttributeAssociation  aa;
   std::string           out;

   aa.source = "";
   aa.target = "target";

   out = aa.check(RegisterContext, XML, "", "", 0);
   EXPECT_STREQ("empty source", out.c_str());

   aa.source = "source";
   aa.target = "";
   out = aa.check(RegisterContext, XML, "", "", 0);
   EXPECT_STREQ("empty target", out.c_str());
}
