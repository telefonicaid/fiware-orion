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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include "unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/ConnectionInfo.h"

extern int servicePathCheck(ConnectionInfo* ciP);



/* ****************************************************************************
*
* rest.servicePathCheck - 
*/
TEST(rest, servicePathCheck)
{
  ConnectionInfo  ci;
  int             r;


  // 0. OK - as no Service Path has been received ...
  ci.servicePath = "";
  r = servicePathCheck(&ci);
  EXPECT_EQ(0, r);

  // Mark that a Service Path has been received
  ci.httpHeaders.servicePathReceived = true;

  // 1. OK
  ci.servicePath = "/h1/_h2/h3/_h4/h5/_h6/h7/_h8/h9/_h10h10h10";
  r = servicePathCheck(&ci);
  EXPECT_EQ(0, r);

  // Not starting with '/'
  ci.servicePath = "x";
  r = servicePathCheck(&ci);
  EXPECT_EQ(1, r);

  // 2. More than 10 components in Service Path
  ci.servicePath = "/h1/h2/h3/h4/h5/h6/h7/h8/h9/h10/h11";
  r = servicePathCheck(&ci);
  EXPECT_EQ(2, r);

  // More than 10 chars in path component of Service Path
  ci.servicePath = "/h0123456789";
  r = servicePathCheck(&ci);
  EXPECT_EQ(3, r);

  // Bad character ('-') in Service Path
  ci.servicePath = "/h-0";
  r = servicePathCheck(&ci);
  EXPECT_EQ(4, r);

}
