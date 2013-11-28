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
#include "rest/OrionError.h"


/* ****************************************************************************
*
* all - 
*/
TEST(OrionError, all)
{
  ErrorCode     ec(SccBadRequest, "Bad Request", "no details");
  StatusCode    sc(SccBadRequest, "Bad Request 2", "no details 2");
  OrionError    e0;
  OrionError    e1(SccOk, "Good Request", "no details 3");
  OrionError    e2(ec);
  OrionError    e3(sc);
  OrionError    e4(SccOk, "Good Request");
  std::string   out;
  std::string   expected1     = "<orionError>\n  <code>200</code>\n  <reasonPhrase>Good Request</reasonPhrase>\n  <details>no details 3</details>\n</orionError>\n";
  std::string   expected2     = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Bad Request</reasonPhrase>\n  <details>no details</details>\n</orionError>\n";
  std::string   expected3     = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Bad Request 2</reasonPhrase>\n  <details>no details 2</details>\n</orionError>\n";
  std::string   expected4     = "<orionError>\n  <code>200</code>\n  <reasonPhrase>Good Request</reasonPhrase>\n</orionError>\n";
  std::string   expected1json = "\"orionError\" : {\n  \"code\" : \"200\",\n  \"reasonPhrase\" : \"Good Request\",\n  \"details\" : \"no details 3\"\n}\n";
  std::string   expected2json = "\"orionError\" : {\n  \"code\" : \"400\",\n  \"reasonPhrase\" : \"Bad Request\",\n  \"details\" : \"no details\"\n}\n";
  std::string   expected3json = "\"orionError\" : {\n  \"code\" : \"400\",\n  \"reasonPhrase\" : \"Bad Request 2\",\n  \"details\" : \"no details 2\"\n}\n";
  std::string   expected4json = "\"orionError\" : {\n  \"code\" : \"200\",\n  \"reasonPhrase\" : \"Good Request\"\n}\n";

  EXPECT_EQ(SccNone, e0.code);
  EXPECT_EQ("",      e0.reasonPhrase);
  EXPECT_EQ("",      e0.details);

  EXPECT_EQ(SccOk,          e1.code);
  EXPECT_EQ("Good Request", e1.reasonPhrase);
  EXPECT_EQ("no details 3", e1.details);

  EXPECT_EQ(ec.code,         e2.code);
  EXPECT_EQ(ec.reasonPhrase, e2.reasonPhrase);
  EXPECT_EQ(ec.details,      e2.details);

  EXPECT_EQ(sc.code,         e3.code);
  EXPECT_EQ(sc.reasonPhrase, e3.reasonPhrase);
  EXPECT_EQ(sc.details,      e3.details);

  EXPECT_EQ(SccOk,          e4.code);
  EXPECT_EQ("Good Request", e4.reasonPhrase);
  EXPECT_EQ("",             e4.details);

  out = e1.render(XML, "");
  EXPECT_EQ(expected1, out);

  out = e2.render(XML, "");
  EXPECT_EQ(expected2, out);

  out = e3.render(XML, "");
  EXPECT_EQ(expected3, out);

  out = e4.render(XML, "");
  EXPECT_EQ(expected4, out);

  out = e1.render(JSON, "");
  EXPECT_EQ(expected1json, out);

  out = e2.render(JSON, "");
  EXPECT_EQ(expected2json, out);

  out = e3.render(JSON, "");
  EXPECT_EQ(expected3json, out);

  out = e4.render(JSON, "");
  EXPECT_EQ(expected4json, out);
}
