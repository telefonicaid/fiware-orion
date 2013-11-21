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

#include "common/Format.h"
#include "convenience/AppendContextElementRequest.h"
#include "ngsi/ContextElement.h"
#include "ngsi/Metadata.h"



/* ****************************************************************************
*
* render - 
*/
TEST(AppendContextElementRequest, render)
{
   AppendContextElementRequest  acer;
   std::string                  out;
   ContextAttribute             ca("caName", "caType", "121");
   Metadata                     md("mdName", "mdType", "122");
   std::string                  expected = "<appendContextElementRequest>\n  <attributeDomainName>ADN</attributeDomainName>\n  <contextAttributeList>\n    <contextAttribute>\n      <name>caName</name>\n      <type>caType</type>\n      <contextValue>121</contextValue>\n    </contextAttribute>\n  </contextAttributeList>\n</appendContextElementRequest>\n";

   acer.attributeDomainName.set("ADN");
   acer.contextAttributeVector.push_back(&ca);
   
   out = acer.render(XML, "");
   EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* check - 
*/
TEST(AppendContextElementRequest, check)
{
   AppendContextElementRequest  acer;
   std::string                  out;
   ContextAttribute             ca("caName", "caType", "121");
   Metadata                     md("mdName", "mdType", "122");
   std::string                  expected1 = "OK";
   std::string                  expected2 = "<appendContextElementResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>Error is predetected</reasonPhrase>\n  </errorCode>\n</appendContextElementResponse>\n";
   std::string                  expected3 = "<appendContextElementResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing attribute name</reasonPhrase>\n  </errorCode>\n</appendContextElementResponse>\n";
   std::string                  expected4 = "<appendContextElementResponse>\n  <errorCode>\n    <code>400</code>\n    <reasonPhrase>missing metadata name</reasonPhrase>\n  </errorCode>\n</appendContextElementResponse>\n";

   acer.attributeDomainName.set("ADN");
   acer.contextAttributeVector.push_back(&ca);
   acer.domainMetadataVector.push_back(&md);

   // 1. ok
   out = acer.check(AppendContextElement, XML, "", "", 0);
   EXPECT_STREQ(expected1.c_str(), out.c_str());

   // 2. Predetected error
   out = acer.check(AppendContextElement, XML, "", "Error is predetected", 0);
   EXPECT_STREQ(expected2.c_str(), out.c_str());
   
   // 3. bad ContextAttribute
   ContextAttribute             ca2("", "caType", "121");

   acer.contextAttributeVector.push_back(&ca2);
   out = acer.check(AppendContextElement, XML, "", "", 0);
   EXPECT_STREQ(expected3.c_str(), out.c_str());
   ca2.name = "ca2Name";

   // 4. Bad domainMetadata
   Metadata                     md2("", "mdType", "122");

   acer.domainMetadataVector.push_back(&md2);
   out = acer.check(AppendContextElement, XML, "", "", 0);
   EXPECT_STREQ(expected4.c_str(), out.c_str());

   // 5. Bad attributeDomainName
   // FIXME P3: AttributeDomainName::check always returns "OK"
}



/* ****************************************************************************
*
* present - just exercise the code
*/
TEST(AppendContextElementRequest, present)
{
   AppendContextElementRequest  acer;
   std::string                  out;
   ContextAttribute             ca("caName", "caType", "121");
   Metadata                     md("mdName", "mdType", "122");

   acer.attributeDomainName.set("ADN");
   acer.contextAttributeVector.push_back(&ca);
   acer.domainMetadataVector.push_back(&md);

   acer.present("");
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

   acer.attributeDomainName.set("ADN");
   acer.contextAttributeVector.push_back(caP);
   acer.domainMetadataVector.push_back(mdP);

   acer.release();
}
