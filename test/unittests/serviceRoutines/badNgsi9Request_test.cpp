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

#include "serviceRoutines/badNgsi9Request.h"
#include "rest/RestService.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "* InvalidRequest",                           "*",      InvalidRequest,                        2, { "ngsi9", "*"                                             }, "", badNgsi9Request                           },
  { "* *",                                        "",       InvalidRequest,                        0, {                                                          }, "", NULL                                      }
};



/* ****************************************************************************
*
* ok - 
*/
TEST(badNgsi9Request, ok)
{
  ConnectionInfo ci("/ngsi9/badbadbad",  "GET", "1.1");
  std::string    out = restService(&ci, rs);
  std::string    expected = "<errorCode>\n  <code>400</code>\n  <reasonPhrase>bad ngsi9 request</reasonPhrase>\n  <details>ngsi9 service '/ngsi9/badbadbad' not found</details>\n</errorCode>\n";

  EXPECT_EQ(expected, out);
}
