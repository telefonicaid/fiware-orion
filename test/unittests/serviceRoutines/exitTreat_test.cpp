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
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>

#include "unittests/unittest.h"

#include "serviceRoutines/exitTreat.h"
#include "rest/RestService.h"
#include "rest/rest.h"



/* ****************************************************************************
*
* harakiri -
*/
extern bool harakiri;



/* ****************************************************************************
*
* getV -
*/
static RestService getV[] =
{
  { ExitRequest,    2, { "exit", "*" }, "", exitTreat },
  { ExitRequest,    1, { "exit"      }, "", exitTreat },
  { InvalidRequest, 0, {             }, "", NULL      }
};



/* ****************************************************************************
*
* error -
*/
TEST(exitTreat, error)
{
  ConnectionInfo ci("/exit/harakiri",  "GET", "1.1");
  std::string    out;

  utInit();

  harakiri = true;
  ci.apiVersion = V1;

  serviceVectorsSet(getV, NULL, NULL, NULL, NULL, NULL, NULL);
  out = orion::requestServe(&ci);

  EXPECT_STREQ("DIE", out.c_str());
  harakiri = false;

  utExit();
}
