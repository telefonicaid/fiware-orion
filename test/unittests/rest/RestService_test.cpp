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
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/postDiscoverContextAvailability.h"
#include "serviceRoutines/badVerbPostOnly.h"

#include "rest/restReply.h"
#include "rest/ConnectionInfo.h"
#include "rest/RestService.h"
#include "rest/rest.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* service routine vectors -
*/
RestService postV[] =
{
  { RegisterContext, 2, { "ngsi9", "registerContext" }, "registerContextRequest", postRegisterContext },
  { InvalidRequest, 0, {}, "", NULL }
};

RestService badVerbs[] =
{
  { RegisterContext, 2, { "ngsi9", "registerContext" }, "registerContextRequest", badVerbPostOnly     },
  { InvalidRequest, 0, {}, "", NULL }
};


#define RC   RegisterContext
#define DCA  DiscoverContextAvailability
#define RCR  "registerContextRequest"
#define DCAR "discoverContextAvailabilityRequest"

RestService postV2[] =
{
  { RC,  2, { "ngsi9",  "registerContext"             }, RCR,  postRegisterContext             },
  { DCA, 2, { "ngsi9",  "discoverContextAvailability" }, DCAR, postDiscoverContextAvailability },
  { InvalidRequest, 0, {}, "", NULL }
};

RestService badVerbs2[] =
{
  { DCA, 2, { "ngsi9",  "discoverContextAvailability" }, DCAR, badVerbPostOnly                 },
  { RC,  2, { "ngsi9",  "registerContext"             }, RCR,  badVerbPostOnly                 },
  { InvalidRequest, 0, {}, "", NULL }
};



/* ****************************************************************************
*
* payloadParse -
*/
TEST(RestService, payloadParse)
{
  ConnectionInfo            ci("/ngsi9/registerContext", "POST", "1.1");
  ParseData                 parseData;
  const char*               infile1  = "ngsi9.registerContext.ok.valid.json";
  std::string               out;
  std::vector<std::string>  compV;
  JsonDelayedRelease        jsonRelease;

  compV.push_back("ngsi9");
  compV.push_back("registerContext");

  utInit();

  //
  // 1. JSON
  //
  EXPECT_EQ("OK", testDataFromFile(testBuf,
                                   sizeof(testBuf),
                                   infile1)) << "Error getting test data from '" << infile1 << "'";

  ci.inMimeType     = JSON;
  ci.outMimeType    = JSON;
  ci.payload        = testBuf;
  ci.payloadSize    = strlen(testBuf);

  out = payloadParse(&ci, &parseData, &postV[0], NULL, &jsonRelease, compV);
  EXPECT_EQ("OK", out);


  //
  // 2. NOMIMETYPE
  //
  EXPECT_EQ("OK", testDataFromFile(testBuf,
                                   sizeof(testBuf),
                                   infile1)) << "Error getting test data from '" << infile1 << "'";

  ci.inMimeType     = NOMIMETYPE;
  ci.outMimeType    = JSON;
  ci.payload        = (char*) "123";
  ci.payloadSize    = strlen(ci.payload);

  out = payloadParse(&ci, &parseData, &postV[0], NULL, &jsonRelease, compV);
  EXPECT_EQ("Bad inMimeType", out);

  utExit();
}



/* ****************************************************************************
*
* noSuchService -
*//*
TEST(RestService, noSuchServiceAndNotFound)
{
  ConnectionInfo ci("/ngsi9/discoverContextAvailability",  "POST", "1.1");
  ci.servicePathV.push_back("");

  const char*    infile    = "ngsi9.discoverContextAvailabilityRequest.ok.valid.json";
  const char*    outfile1  = "ngsi9.discoverContextAvailabilityRsponse.serviceNotRecognized.valid.json";
  const char*    outfile2  = "ngsi9.discoverContextAvailabilityRsponse.notFound.valid.json";
  std::string    out;

  utInit();

  // No such service
  EXPECT_EQ("OK", testDataFromFile(testBuf,
                                   sizeof(testBuf),
                                   infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile1)) << "Error getting test data from '" << outfile1 << "'";

  ci.outMimeType    = JSON;
  ci.inMimeType     = JSON;
  ci.payload        = testBuf;
  ci.payloadSize    = strlen(testBuf);

  serviceVectorsSet(NULL, NULL, postV, NULL, NULL, NULL, badVerbs);
  out = orion::requestServe(&ci);
  EXPECT_STREQ(expectedBuf, out.c_str());

  // Not found
  EXPECT_EQ("OK", testDataFromFile(testBuf,
                                   sizeof(testBuf),
                                   infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf,
                                   sizeof(expectedBuf),
                                   outfile2)) << "Error getting test data from '" << outfile2 << "'";

  ci.outMimeType    = JSON;
  ci.inMimeType     = JSON;
  ci.payload        = testBuf;
  ci.payloadSize    = strlen(testBuf);

  serviceVectorsSet(NULL, NULL, postV2, NULL, NULL, NULL, badVerbs2);
  out = orion::requestServe(&ci);

  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}*/
