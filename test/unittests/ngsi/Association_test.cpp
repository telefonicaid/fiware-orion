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
#include "unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "ngsi/Association.h"



/* ****************************************************************************
*
* ok - 
*/
TEST(Association, ok)
{
   Association           a;
   AttributeAssociation* aa       = new AttributeAssociation();
   const char*           outfile  = "ngsi.association.ok.middle.xml";
   std::string           out;

   utInit();

   a.entityAssociation.source.fill("Source", "source", "false");
   a.entityAssociation.target.fill("Target", "target", "false");

   aa->source = "Source";
   aa->target = "Target";

   a.attributeAssociationList.push_back(aa);

   out = a.check(RegisterContext, XML, "", "", 0);
   EXPECT_STREQ("OK", out.c_str());

   EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
   out = a.render(XML, "", false);
   EXPECT_STREQ(expectedBuf, out.c_str());

   a.release();

   utExit();
}
