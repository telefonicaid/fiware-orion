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

#include "common/MimeType.h"
#include "convenience/AppendContextElementRequest.h"
#include "ngsi/Metadata.h"
#include "rest/ConnectionInfo.h"

#include "unittest.h"



/* ****************************************************************************
*
* render_json -
*/
TEST(AppendContextElementRequest, render_json)
{
   AppendContextElementRequest  acer;
   std::string                  out;
   ContextAttribute             ca("caName", "caType", "121");
   const char*                  outfile = "ngsi10.appendContextElementRequest.adn.valid.json";


   utInit();

   acer.contextAttributeVector.push_back(&ca);
   
   out = acer.render(false, UpdateContext);

   EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
   EXPECT_STREQ(expectedBuf, out.c_str());

   utExit();
}



/* ****************************************************************************
*
* check_json -
*/
TEST(AppendContextElementRequest, check_json)
{
   AppendContextElementRequest  acer;
   std::string                  out;
   ContextAttribute             ca("caName", "caType", "121");
   const char*                  outfile1 = "ngsi10.appendContextElementResponse.predetectedError.valid.json";
   const char*                  outfile2 = "ngsi10.appendContextElementResponse.missingAttributeName.valid.json";

   utInit();

   acer.contextAttributeVector.push_back(&ca);

   // 1. ok
   out = acer.check(V1, false, AppendContextElement, "");
   EXPECT_STREQ("OK", out.c_str());


   // 2. Predetected error
   EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
   out = acer.check(V1, false, AppendContextElement, "Error is predetected");
   EXPECT_STREQ(expectedBuf, out.c_str());


   // 3. bad ContextAttribute
   ContextAttribute  ca2("", "caType", "121");

   acer.contextAttributeVector.push_back(&ca2);
   out = acer.check(V1, false, AppendContextElement, "");
   EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";   
   EXPECT_STREQ(expectedBuf, out.c_str());
   ca2.name = "ca2Name";

   utExit();
}



/* ****************************************************************************
*
* release - just exercise the code
*/
TEST(AppendContextElementRequest, release)
{
   AppendContextElementRequest  acer;
   std::string                  out;
   ContextAttribute*            caP = new ContextAttribute("caName", "caType", "121");
   Metadata*                    mdP = new Metadata("mdName", "mdType", "122");

   acer.contextAttributeVector.push_back(caP);

   acer.release();
}
