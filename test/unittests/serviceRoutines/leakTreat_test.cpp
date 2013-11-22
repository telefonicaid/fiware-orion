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

#include "serviceRoutines/leakTreat.h"
#include "rest/RestService.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "GET LeakRequest",                            "GET",    LeakRequest,                           2, { "leak", "*"                                              }, "", leakTreat                                 },
  { "GET LeakRequest",                            "GET",    LeakRequest,                           1, { "leak"                                                   }, "", leakTreat                                 },
  { "* *",                                        "",       InvalidRequest,                        0, {                                                          }, "", NULL                                      }
};



/* ****************************************************************************
*
* error - 
*/
TEST(leakTreat, error)
{
  ConnectionInfo ci1("/leak",  "GET", "1.1");
  ConnectionInfo ci2("/leak/nadadenada",  "GET", "1.1");
  ConnectionInfo ci3("/leak/harakiri",  "GET", "1.1");

  extern bool harakiri;
  harakiri = true;

  std::string    expected1  = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Bad request</reasonPhrase>\n  <details>Password requested</details>\n</orionError>\n";
  std::string    expected2  = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Bad request</reasonPhrase>\n  <details>Request denied - password erroneous</details>\n</orionError>\n";

  std::string    out1       = restService(&ci1, rs);
  std::string    out2       = restService(&ci2, rs);

  EXPECT_STREQ(expected1.c_str(), out1.c_str());
  EXPECT_STREQ(expected2.c_str(), out2.c_str());

  harakiri = false;
  std::string    expected3  = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Bad request</reasonPhrase>\n  <details>no such service</details>\n</orionError>\n";
  std::string    out3       = restService(&ci3, rs);
  EXPECT_STREQ(expected3.c_str(), out3.c_str());
}
