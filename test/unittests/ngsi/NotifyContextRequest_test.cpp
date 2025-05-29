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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "rest/OrionError.h"
#include "ngsi/NotifyContextRequest.h"
#include "ngsi/NotifyContextResponse.h"

#include "unittest.h"



/* ****************************************************************************
*
* json_render -
*/
TEST(NotifyContextRequest, json_render)
{
  utInit();

  const char*              filename1  = "ngsi10.notifyContextRequest.jsonRender1.valid.json";
  const char*              filename2  = "ngsi10.notifyContextRequest.jsonRender2.valid.json";
  const char*              filename3  = "ngsi10.notifyContextRequest.jsonRender3.valid.json";
  NotifyContextRequest*    ncrP;
  ContextElementResponse*  cerP;
  std::string              rendered;  

  // Preparation
  ncrP = new NotifyContextRequest();
  ncrP->subscriptionId = "012345678901234567890123";

  std::vector<std::string> emptyV;

  // 1. Without ContextResponseList
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename1)) << "Error getting test data from '" << filename1 << "'";
  rendered = ncrP->toJson(NGSI_V2_NORMALIZED, emptyV, false, emptyV, NULL);
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 2. With ContextResponseList
  cerP = new ContextElementResponse();
  EntityId enId1("E01", "", "EType", "");
  cerP->entity.fill(enId1);
  ncrP->contextElementResponseVector.push_back(cerP);
  cerP->error.fill(SccOk);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename2)) << "Error getting test data from '" << filename2 << "'";
  rendered = ncrP->toJson(NGSI_V2_NORMALIZED, emptyV, false, emptyV, NULL);
  EXPECT_STREQ(expectedBuf, rendered.c_str());


  // 3. ContextResponseList with two instances
  cerP = new ContextElementResponse();
  EntityId enId2("E02", "", "EType", "");
  cerP->entity.fill(enId2);
  ncrP->contextElementResponseVector.push_back(cerP);
  cerP->error.fill(SccOk);

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), filename3)) << "Error getting test data from '" << filename3 << "'";
  rendered = ncrP->toJson(NGSI_V2_NORMALIZED, emptyV, false, emptyV, NULL);
  EXPECT_STREQ(expectedBuf, rendered.c_str());

  utExit();
}
