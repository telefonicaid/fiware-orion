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

#include "serviceRoutines/exitTreat.h"
#include "rest/RestService.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "GET ExitRequest",                            "GET",    ExitRequest,                           2, { "exit", "*"                                              }, "", exitTreat                                 },
  { "GET ExitRequest",                            "GET",    ExitRequest,                           1, { "exit"                                                   }, "", exitTreat                                 },
  { "* *",                                        "",       InvalidRequest,                        0, {                                                          }, "", NULL                                      }
};



/* ****************************************************************************
*
* error - 
*/
TEST(exitTreat, error)
{
  ConnectionInfo ci1("/exit",  "GET", "1.1");
  ConnectionInfo ci2("/exit/nadadenada",  "GET", "1.1");
  ConnectionInfo ci3("/exit/harakiri",  "GET", "1.1");

  std::string    expected1  = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Bad request</reasonPhrase>\n  <details>Password requested</details>\n</orionError>\n";
  std::string    expected2  = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Bad request</reasonPhrase>\n  <details>Request denied - password erroneous</details>\n</orionError>\n";

  std::string    out1       = restService(&ci1, rs);
  std::string    out2       = restService(&ci2, rs);

  EXPECT_STREQ(expected1.c_str(), out1.c_str());
  EXPECT_STREQ(expected2.c_str(), out2.c_str());

#if 0
  // I CANT RUN THIS TEST AS IT KILLS THE BROKER
  // The test is taken care of by harness tests
  std::string    expected3  = "<orion>\n  <exit>Exit requested so I obey and exit</exit>\n</orion>\n";
  std::string    out3       = restService(&ci3, rs);
  EXPECT_STREQ(expected3.c_str(), out3.c_str());
#endif
}
