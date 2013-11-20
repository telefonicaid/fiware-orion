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
#include "convenience/UpdateContextAttributeRequest.h"
#include "convenience/ContextAttributeResponseVector.h"



/* ****************************************************************************
*
* render - 
*/
TEST(UpdateContextAttributeRequest, render)
{
  UpdateContextAttributeRequest  ucar;
  Metadata                       mdata("name", "type", "value");
  std::string                    out;
  std::string                    expected = "<updateContexAttributeRequest>\n  <type>TYPE</type>\n  <contextValue>Context Value</contextValue>\n  <registrationMetadata>\n    <contextMetadata>\n      <name>name</name>\n      <type>type</type>\n      <value>value</value>\n    </contextMetadata>\n  </registrationMetadata>\n</updateContexAttributeRequest>\n";

  ucar.type         = "TYPE";
  ucar.contextValue = "Context Value";

  ucar.metadataVector.push_back(&mdata);
  out = ucar.render(XML, "");
  EXPECT_STREQ(expected.c_str(), out.c_str());
}



/* ****************************************************************************
*
* check - 
*/
TEST(UpdateContextAttributeRequest, check)
{
  UpdateContextAttributeRequest  ucar;
  Metadata                       mdata("name", "type", "value");
  Metadata                       mdata2("", "type", "value");
  std::string                    out;
  std::string                    expected1 = "<statusCode>\n  <code>400</code>\n  <reasonPhrase>PRE Error</reasonPhrase>\n</statusCode>\n";
  std::string                    expected2 = "<statusCode>\n  <code>400</code>\n  <reasonPhrase>empty context value</reasonPhrase>\n</statusCode>\n";
  std::string                    expected3 = "OK";
  std::string                    expected4 = "<statusCode>\n  <code>400</code>\n  <reasonPhrase>missing metadata name</reasonPhrase>\n</statusCode>\n";

  ucar.metadataVector.push_back(&mdata);

  // 1. predetectedError + Format ZERO => XML
  out = ucar.check(UpdateContextAttribute, (Format) 0, "", "PRE Error", 0);
  EXPECT_STREQ(expected1.c_str(), out.c_str());
  
  // 2. empty contextValue
  out = ucar.check(UpdateContextAttribute, XML, "", "", 0);
  EXPECT_STREQ(expected2.c_str(), out.c_str());

  // 3. OK
  ucar.contextValue = "CValue";
  out = ucar.check(UpdateContextAttribute, XML, "", "", 0);
  EXPECT_STREQ(expected3.c_str(), out.c_str());

  // 4. bad metadata
  ucar.metadataVector.push_back(&mdata2);
  out = ucar.check(UpdateContextAttribute, XML, "", "", 0);
  EXPECT_STREQ(expected4.c_str(), out.c_str());
}



/* ****************************************************************************
*
* present - just exercise the code
*/
TEST(UpdateContextAttributeRequest, present)
{
  UpdateContextAttributeRequest  ucar;
  Metadata                       mdata("name", "type", "value");

  ucar.metadataVector.push_back(&mdata);

  ucar.present("");
}



/* ****************************************************************************
*
* release - 
*/
TEST(UpdateContextAttributeRequest, release)
{
  UpdateContextAttributeRequest  ucar;
  Metadata*                      mdataP = new Metadata("name", "type", "value");

  ucar.metadataVector.push_back(mdataP);
  ASSERT_EQ(1, ucar.metadataVector.size());
  ucar.release();
  EXPECT_EQ(0, ucar.metadataVector.size());
}
