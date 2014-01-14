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

#include "serviceRoutines/deleteIndividualContextEntityAttributes.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "DELETE", IndividualContextEntityAttributes,     4, { "ngsi10", "contextEntities", "*", "attributes"       }, "", deleteIndividualContextEntityAttributes   },
  { "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* notFound - 
*/
TEST(deleteIndividualContextEntityAttributes, notFound)
{
  ConnectionInfo  ci("/ngsi10/contextEntities/entity901/attributes",  "DELETE", "1.1");
  std::string     expected = "<statusCode>\n  <code>404</code>\n  <reasonPhrase>No context element found</reasonPhrase>\n  <details>entity901</details>\n</statusCode>\n";
  std::string     out      = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());
}
