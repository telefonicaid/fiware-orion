/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/ConnectionInfo.h"

#include "orionld/types/ApiVersion.h"
#include "unittests/unittest.h"



extern int servicePathCheck(ConnectionInfo* ciP, const char* path);
extern int servicePathSplit(ConnectionInfo* ciP);



/* ****************************************************************************
*
* rest.servicePathCheck -
*/
TEST(rest, servicePathCheck)
{
  ConnectionInfo  ci;
  int             r;

  // 0. OK - as no Service Path has been received ...
  r = servicePathCheck(&ci, NULL);
  EXPECT_EQ(0, r);

  // Mark that a Service Path has been received

  // 1. OK
  r = servicePathCheck(&ci, "/h1/_h2/h3/_h4/h5/_h6/h7/_h8/h9/_h10h10h10");
  EXPECT_EQ(0, r);

  // Not starting with '/'
  r = servicePathCheck(&ci, "x");
  EXPECT_EQ(1, r);

  // 2. More than 10 components in Service Path
  r = servicePathCheck(&ci, "/h1/h2/h3/h4/h5/h6/h7/h8/h9/h10/h11");
  EXPECT_EQ(2, r);

  // More than 50 chars in path component of Service Path
  r = servicePathCheck(&ci, "/h01234567890123456789012345678901234567890123456789");
  EXPECT_EQ(3, r);

  // Bad character ('-') in Service Path
  r = servicePathCheck(&ci, "/h-0");
  EXPECT_EQ(4, r);
}



/* ****************************************************************************
*
* rest.servicePathSplit -
*/
TEST(rest, servicePathSplit)
{
  ConnectionInfo  ci1;
  ConnectionInfo  ci2;
  ConnectionInfo  ci3;
  ConnectionInfo  ci4;
  ConnectionInfo  ci5;
  int             r;

  orionldState.apiVersion = API_VERSION_NGSI_V1;

  // 1. OK - as no Service Path has been received ...
  LM_M(("---- 1 -----"));
  orionldState.in.servicePath = (char*) "";
  r = servicePathSplit(&ci1);
  EXPECT_EQ(0, r);
  LM_M(("---- 1 -----"));


  // 2. OK - one service path
  LM_M(("---- 2 -----"));
  orionldState.in.servicePath = (char*) "/h1/_h2/h3/_h4/h5/_h6/h7/_h8/h9/_h10h10h10";
  r = servicePathSplit(&ci2);
  EXPECT_EQ(0, r);
  LM_M(("---- 2 -----"));

  // 3. OK - two service paths
  LM_M(("---- 3 -----"));
  orionldState.in.servicePath = (char*) "/h1/_h2/h3/_h4/h5/_h6/h7/_h8/h9/_h10h10h10, /1/2/3";
  r = servicePathSplit(&ci3);
  EXPECT_EQ(0, r);
  EXPECT_STREQ("", ci3.answer.c_str());
  LM_M(("---- 3 -----"));

  // 4. OK - nine service paths
  LM_M(("---- 4 -----"));
  orionldState.in.servicePath = (char*) "/home/kz/01, /home/kz/02, /home/kz/03, /home/kz/04, /home/kz/05, /home/kz/06, /home/kz/07, /home/kz/08, /home/kz/09";
  r = servicePathSplit(&ci4);
  EXPECT_EQ(0, r);
  EXPECT_STREQ("", ci4.answer.c_str());
  LM_M(("---- 4 -----"));

  // 5. NOT OK - eleven service paths
  LM_M(("---- 5 -----"));
  orionldState.in.servicePath = (char*) "/home/kz/01, /home/kz/02, /home/kz/03, /home/kz/04, /home/kz/05, /home/kz/06, /home/kz/07, /home/kz/08, /home/kz/09, /home/kz/10, /home/kz/11";
  r = servicePathSplit(&ci5);
  EXPECT_EQ(-1, r);
  EXPECT_EQ(137, ci5.answer.size());
  LM_M(("---- 5 -----"));
}
