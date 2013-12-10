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
* harakiri - 
*/
extern bool harakiri;



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "GET",    ExitRequest,                           2, { "exit", "*"                                              }, "", exitTreat                                 },
  { "GET",    ExitRequest,                           1, { "exit"                                                   }, "", exitTreat                                 },
  { "",       InvalidRequest,                        0, {                                                          }, "", NULL                                      }
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
  std::string    expected3  = "<orionError>\n  <code>400</code>\n  <reasonPhrase>Bad request</reasonPhrase>\n  <details>no such service</details>\n</orionError>\n";
  std::string    out;

  harakiri = true;

  out = restService(&ci1, rs);
  EXPECT_EQ(expected1, out);

  out =restService(&ci2, rs);
  EXPECT_EQ(expected2, out);

  harakiri = false;

  out = restService(&ci3, rs);
  EXPECT_EQ(expected3, out);
}
