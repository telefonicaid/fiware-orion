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

#include "common/globals.h"

#include "serviceRoutines/postIndividualContextEntity.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "testDataFromFile.h"

#include "commonMocks.h"

using ::testing::Throw;
using ::testing::Return;



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST",   IndividualContextEntity,               3, { "ngsi10", "contextEntities", "*"                     }, "", postIndividualContextEntity               },
  { "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
*
* ok - 
*/
TEST(postIndividualContextEntity, ok)
{
  ConnectionInfo ci("/ngsi10/contextEntities/entity11",  "POST", "1.1");  
  std::string    expected      = "<appendContextElementResponse>\n  <contextResponseList>\n    <contextAttributeResponse>\n      <contextAttributeList>\n        <contextAttribute>\n          <name>pressure</name>\n          <type>clima</type>\n          <contextValue></contextValue>\n        </contextAttribute>\n      </contextAttributeList>\n      <statusCode>\n        <code>200</code>\n        <reasonPhrase>OK</reasonPhrase>\n      </statusCode>\n    </contextAttributeResponse>\n  </contextResponseList>\n</appendContextElementResponse>\n";
  const char*    fileName      = "ngsi10.appendContextElementRequest.ok.valid.xml";
  std::string    out;

  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), fileName)) << "Error getting test data from '" << fileName << "'";

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

  delete timerMock;
  setTimer(NULL);
}
