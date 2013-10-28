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

#include "ngsi10/QueryContextResponse.h"



/* ****************************************************************************
*
* ok_xml - 
*/
TEST(QueryContextResponse, ok_xml)
{
  ErrorCode*            ecP = new ErrorCode(SccOk, "Reason", "Detail");
  ErrorCode             ec(SccOk, "Reason2", "Detail2");
  QueryContextResponse  qcr1;
  QueryContextResponse  qcr2(ec);
  std::string           rendered;
  std::string           expected = "<queryContextResponse>\n  <errorCode>\n    <code>200</code>\n    <reasonPhrase>Reason2</reasonPhrase>\n    <details>Detail2</details>\n  </errorCode>\n</queryContextResponse>\n";

  rendered = qcr2.render(QueryContext, XML, "");
  EXPECT_STREQ(expected.c_str(), rendered.c_str());

  delete ecP;
}
