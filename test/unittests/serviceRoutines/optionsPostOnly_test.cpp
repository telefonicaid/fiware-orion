/*
*
* Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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
* iot_support at tid dot es
*
* Author: Burak Karaboga - ATOS Research & Innovation
*/
#include <string>

#include "gtest/gtest.h"

#include "serviceRoutinesV2/optionsPostOnly.h"
#include "rest/RestService.h"
#include "rest/rest.h"



/* ****************************************************************************
*
* optionsV -
*/
static RestService optionsV[] =
{
  { BatchQueryRequest,  3, { "v2", "op", "query" }, "", optionsPostOnly },
  { InvalidRequest,     0, {                     }, "", NULL            }
};



/* ****************************************************************************
*
* ok -
*/
TEST(versionTreat, ok)
{
  ConnectionInfo  ci("/v2/op/query",  "OPTIONS", "1.1");
  std::string     out;

  serviceVectorsSet(NULL, NULL, NULL, NULL, NULL, optionsV, NULL);
  out = orionServe(&ci);

  EXPECT_TRUE(strstr(out.c_str(), "Access-Control-Allow-Origin")  != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "Access-Control-Max-Age")       != NULL);
  EXPECT_TRUE(strstr(out.c_str(), "Access-Control-Allow-Headers") != NULL);

  std::string expected = std::string("Access-Control-Allow-Methods: POST, OPTIONS");
  EXPECT_TRUE(strstr(out.c_str(), expected.c_str()) != NULL);
}
