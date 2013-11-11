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

#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "testDataFromFile.h"
#include "commonMocks.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST UpdateContext",                         "POST",   UpdateContext,                         2, { "ngsi10", "updateContext"                            }, "", postUpdateContext                         },
  { "* InvalidRequest",                           "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "* *",                                        "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* notFound - 
*/
TEST(postUpdateContext, notFound)
{
  ConnectionInfo ci("/ngsi10/updateContext",  "POST", "1.1");
  std::string    expected    = "<updateContextResponse>\n  <contextResponseList>\n    <contextElementResponse>\n      <contextElement>\n        <entityId type=\"Room\" isPattern=\"false\">\n          <id>ConferensRoom</id>\n        </entityId>\n      </contextElement>\n      <statusCode>\n        <code>404</code>\n        <reasonPhrase>Entity not found</reasonPhrase>\n        <details>entity: (ConferensRoom, Room, false)</details>\n      </statusCode>\n    </contextElementResponse>\n  </contextResponseList>\n</updateContextResponse>\n";
  const char*    fileName    = "ngsi10.updateContextRequest.idNotFound.valid.xml";
  std::string    out;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

  NotifierMock* notifierMock = new NotifierMock();
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  ci.outFormat    = XML;
  ci.inFormat     = XML;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs);

  EXPECT_STREQ(expected.c_str(), out.c_str());

  delete notifierMock;
  delete timerMock;
}
