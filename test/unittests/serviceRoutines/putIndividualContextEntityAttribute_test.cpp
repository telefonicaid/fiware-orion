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
* Author: TID Developer
*/
#include "logMsg/logMsg.h"

#include "serviceRoutines/putIndividualContextEntityAttribute.h"
#include "serviceRoutines/badRequest.h"
#include "rest/RestService.h"

#include "unittest.h"



/* ****************************************************************************
*
* rs - 
*/
static RestService rs[] = 
{
  { "PUT",    IndividualContextEntityAttribute,      5, { "ngsi10", "contextEntities", "*", "attributes", "*"  }, "", putIndividualContextEntityAttribute      },
  { "*",      InvalidRequest,                        0, { "*", "*", "*", "*", "*", "*"                         }, "", badRequest                                },
  { "",       InvalidRequest,                        0, {                                                      }, "", NULL                                      }
};



/* ****************************************************************************
 * 
 * json - 
 */
TEST(putIndividualContextEntityAttribute, json)
{
  ConnectionInfo ci("/ngsi10/contextEntities/entity11/attributes/temperature",  "PUT", "1.1");
  ci.servicePathV.push_back("");

  const char*    infile      = "ngsi10.updateContextAttributeRequest.putAttribute.valid.json";
  const char*    outfile     = "ngsi10.updateContextAttributeResponse.notFound.valid.json";
  std::string    out;
  
  EXPECT_EQ("OK", testDataFromFile(testBuf, sizeof(testBuf), infile)) << "Error getting test data from '" << infile << "'";
  EXPECT_EQ("OK", testDataFromFile(expectedBuf, sizeof(expectedBuf), outfile)) << "Error getting test data from '" << outfile << "'";
  
  ci.outFormat    = JSON;
  ci.inFormat     = JSON;
  ci.payload      = testBuf;
  ci.payloadSize  = strlen(testBuf);
  out             = restService(&ci, rs);
  
  EXPECT_STREQ(expectedBuf, out.c_str());
  
  utExit();
}
