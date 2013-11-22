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

#include "convenience/Convenience.h"

#include "common/Timer.h"
#include "common/globals.h"

#include "rest/RestService.h"
#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/getContextEntitiesByEntityId.h"



/* ****************************************************************************
*
* restServiceV - 
*/
RestService restServiceV[] =
{
  { "POST RegisterContext",                       "POST",   RegisterContext,                       2, { "ngsi9",  "registerContext"                       }, "", postRegisterContext                       },
  { "GET ContextEntitiesByEntityId",              "GET",    ContextEntitiesByEntityId,             3, { "ngsi9", "contextEntities", "*"                   }, "", getContextEntitiesByEntityId              },
  { "InvalidRequest",                             "",       InvalidRequest,                        0, {                                                   }, "", NULL                                      }
};



/* ****************************************************************************
*
* emptyPath - 
*/
TEST(Convenience, emptyPath)
{
  ConnectionInfo            ci("", "GET", "1.1");
  std::string               response;
  std::string               expected = "Empty URL";

  response = restService(&ci, restServiceV);
  EXPECT_STREQ(expected.c_str(), response.c_str());
}



/* ****************************************************************************
*
* shortPath - 
*/
TEST(Convenience, shortPath)
{
  ConnectionInfo  ci1("ngsi9", "GET", "1.1");
  ConnectionInfo  ci2("ngsi10", "GET", "1.1");
  ConnectionInfo  ci3("ngsi8", "GET", "1.1");
  ConnectionInfo  ci4("ngsi10/nada", "GET", "1.1");
  std::string     out;
  std::string     expected = "<orionError>\n  <code>400</code>\n  <reasonPhrase>bad request</reasonPhrase>\n  <details>Service not recognized</details>\n</orionError>\n";

  out = restService(&ci1, restServiceV);
  EXPECT_EQ(expected, out);

  out = restService(&ci2, restServiceV);
  EXPECT_EQ(expected, out);

  out = restService(&ci3, restServiceV);
  EXPECT_EQ(expected, out);

  out = restService(&ci4, restServiceV);
  EXPECT_EQ(expected, out);
}



/* ****************************************************************************
*
* badPathNgsi9 - 
*/
TEST(Convenience, badPathNgsi9)
{
  ConnectionInfo            ci("ngsi9/badpathcomponent", "GET", "1.1");
  std::string               out;
  std::string               expected = "<orionError>\n  <code>400</code>\n  <reasonPhrase>bad request</reasonPhrase>\n  <details>Service not recognized</details>\n</orionError>\n";

  out = restService(&ci, restServiceV);
  EXPECT_EQ(expected, out);
}



/* ****************************************************************************
*
* badPathNgsi10 - 
*/
TEST(Convenience, badPathNgsi10)
{
  ConnectionInfo            ci("ngsi10/badpathcomponent", "GET", "1.1");
  std::string               out;
  std::string               expected = "<orionError>\n  <code>400</code>\n  <reasonPhrase>bad request</reasonPhrase>\n  <details>Service not recognized</details>\n</orionError>\n";

  out = restService(&ci, restServiceV);
  EXPECT_EQ(expected, out);
}
