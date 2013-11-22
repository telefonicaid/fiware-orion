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
#include "logMsg/traceLevels.h"

#include "jsonParse/jsonRequest.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* - 
*/
TEST(jsonRequest, jsonTreat)
{
   ConnectionInfo  ci("/ngsi9/registerContext", "POST", "1.1");
   ParseData       parseData;
   
   ci.outFormat = JSON;

   std::string  out;
   std::string  expected1 = "\"orionError\" : {\n  \"code\" : \"400\",\n  \"reasonPhrase\" : \"no request treating object found\",\n  \"details\" : \"Sorry, no request treating object found for RequestType 'InvalidRequest'\"\n}\n";
   std::string  expected2 = "\"orionError\" : {\n  \"code\" : \"400\",\n  \"reasonPhrase\" : \"Sorry, not implemented\"\n}\n";

   out  = jsonTreat("", &ci, &parseData, InvalidRequest, "no_payload", NULL);
   EXPECT_EQ(expected1, out);
}
