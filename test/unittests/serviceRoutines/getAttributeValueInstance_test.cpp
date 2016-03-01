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

#include "serviceRoutines/postIndividualContextEntity.h"
#include "serviceRoutines/getAttributeValueInstance.h"
#include "serviceRoutines/putAttributeValueInstance.h"
#include "serviceRoutines/deleteAttributeValueInstance.h"
#include "serviceRoutines/badVerbGetPutDeleteOnly.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "POST",   IndividualContextEntityAttributes, 4, { "ngsi10", "contextEntities", "*", "attributes"           }, "appendContextElementRequest",   postIndividualContextEntity           },
  { "GET",    AttributeValueInstance,            6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                              getAttributeValueInstance             },
  { "PUT",    AttributeValueInstance,            6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "updateContextAttributeRequest", putAttributeValueInstance             },
  { "DELETE", AttributeValueInstance,            6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                              deleteAttributeValueInstance          },
  { "*",      AttributeValueInstance,            6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                              badVerbGetPutDeleteOnly               },
  { "*",      InvalidRequest,                    0, { "*", "*", "*", "*", "*", "*"                             }, "",                              badRequest                            },
  { "",       InvalidRequest,                    0, {                                                          }, "",                              NULL                                  }
};



/* ****************************************************************************
*
* notFound - 
*
* FIXME P5 #1862: _json counterpart?
*/
TEST(getAttributeValueInstance, DISABLED_notFound)
{
  ConnectionInfo ci("/ngsi10/contextEntities/E1/attributes/A1/left",  "GET", "1.1");
  const char*    outfile = "ngsi10.contextAttributeResponse.notFound.valid.xml";
  std::string    out;

  utInit();
    
  ci.outFormat = XML;
  out          = restService(&ci, rs);
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}



/* ****************************************************************************
*
* found - 
*
* FIXME P5 #1862: _json counterpart?
*/
TEST(getAttributeValueInstance, DISABLED_found)
{
  ConnectionInfo ci1("/ngsi10/contextEntities/E1/attributes",          "POST", "1.1");
  ConnectionInfo ci2("/ngsi10/contextEntities/E1/attributes/A1/left",  "GET", "1.1");
  const char*    outfile1  = "ngsi10.appendContextElementResponse.ok1.valid.xml";
  const char*    outfile2  = "ngsi10.appendContextElementResponse.ok2.valid.xml";
  const char*    infile    = "ngsi10.IndividualContextEntityAttributes.A1-left.postponed.xml";
  std::string    out;

  utInit();

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile1)) << "Error getting test data from '" << outfile1 << "'";
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";

  ci1.outFormat    = XML;
  ci1.inFormat     = XML;
  ci1.payload      = testBuf;
  ci1.payloadSize  = strlen(testBuf);
  out              = restService(&ci1, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile2)) << "Error getting test data from '" << outfile2 << "'";
  ci2.outFormat    = XML;
  out              = restService(&ci2, rs);
  EXPECT_STREQ(expectedBuf, out.c_str());

  utExit();
}
