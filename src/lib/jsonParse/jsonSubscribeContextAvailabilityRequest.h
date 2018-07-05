#ifndef SRC_LIB_JSONPARSE_JSONSUBSCRIBECONTEXTAVAILABILITYREQUEST_H_
#define SRC_LIB_JSONPARSE_JSONSUBSCRIBECONTEXTAVAILABILITYREQUEST_H_

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

#include "rest/ConnectionInfo.h"
#include "jsonParse/JsonNode.h"



/* ****************************************************************************
*
* jsonScarParseVector -
*/
extern JsonNode jsonScarParseVector[];



/* ****************************************************************************
*
* jsonScarInit - 
*/
extern void jsonScarInit(ParseData* reqDataP);



/* ****************************************************************************
*
* jsonScarRelease - 
*/
extern void jsonScarRelease(ParseData* reqDataP);



/* ****************************************************************************
*
* jsonScarCheck - 
*/
extern std::string jsonScarCheck(ParseData* reqDataP, ConnectionInfo* ciP);

#endif  // SRC_LIB_JSONPARSE_JSONSUBSCRIBECONTEXTAVAILABILITYREQUEST_H_
