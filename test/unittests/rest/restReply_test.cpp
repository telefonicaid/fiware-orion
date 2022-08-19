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
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "rest/restReply.h"
#include "rest/ConnectionInfo.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* MHD_create_response_from_data_error -
*
* Too large response string
*/
#define TEST_SIZE (4 * 1024 * 1024)
TEST(restReply, MHD_create_response_from_data_error)
{
  ConnectionInfo  ci("/ngsi/XXX", "GET", "1.1");
  char*           answer = (char*) malloc(TEST_SIZE);

  utInit();

  if (answer != NULL)
  {
    memset(answer, 'x', TEST_SIZE - 1);
    answer[TEST_SIZE - 1] = 0;

    restReply(&ci, answer);
    free(answer);
  }

  utExit();
}



/* ****************************************************************************
*
* json -
*/
TEST(restReply, json)
{
  ConnectionInfo  ci("/ngsi/XXX", "GET", "1.1");

  utInit();

  ci.outMimeType = JSON;
  restReply(&ci, "123");

  utExit();
}
