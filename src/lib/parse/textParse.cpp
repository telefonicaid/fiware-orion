/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "orionld/common/orionldState.h"             // orionldState

#include "common/string.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* textParseAttributeValue -
*/
static std::string textParseAttributeValue(ConnectionInfo* ciP, ContextAttribute* caP)
{
  double d;

  // 1. Starts and ends with citation marks?
  if (orionldState.in.payload[0] == '"')
  {
    char* end = &orionldState.in.payload[strlen(orionldState.in.payload) - 1];

    if (*end == '"')
    {
      *end = 0;
      caP->stringValue = &orionldState.in.payload[1];
      caP->valueType   = orion::ValueTypeString;
    }
    else
    {
      OrionError oe(SccBadRequest, "Missing citation-mark at end of string");
      return oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);
    }
  }

  // 2. True or false?
  else if ((strlen(orionldState.in.payload) == 4) && ((strcmp(orionldState.in.payload, "true") == 0) || (strcmp(orionldState.in.payload, "True") == 0) || (strcmp(orionldState.in.payload, "TRUE") == 0)))
  {
    caP->boolValue   = true;
    caP->valueType   = orion::ValueTypeBoolean;
  }
  else if ((strlen(orionldState.in.payload) == 5) && ((strcmp(orionldState.in.payload, "false") == 0) || (strcmp(orionldState.in.payload, "False") == 0) || (strcmp(orionldState.in.payload, "FALSE") == 0)))
  {
    caP->boolValue   = false;
    caP->valueType   = orion::ValueTypeBoolean;
  }

  // 3. Null ?
  else if ((strlen(orionldState.in.payload) == 4) && ((strcmp(orionldState.in.payload, "null") == 0) || (strcmp(orionldState.in.payload, "Null") == 0) || (strcmp(orionldState.in.payload, "NULL") == 0)))
  {
    caP->valueType   = orion::ValueTypeNull;
  }

  // 4. Is it a valid double?
  else if (str2double(orionldState.in.payload, &d) == true)
  {
    caP->valueType   = orion::ValueTypeNumber;
    caP->numberValue = d;
  }

  else  // 5. None of the above - it's an error
  {
    OrionError oe(SccBadRequest, "attribute value type not recognized");
    return oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);
  }

  return "OK";
}



/* ****************************************************************************
*
* textRequestTreat -
*/
std::string textRequestTreat(ConnectionInfo* ciP, ParseData* parseDataP, RequestType requestType)
{
  std::string answer;

  switch (requestType)
  {
  case EntityAttributeValueRequest:
    answer = textParseAttributeValue(ciP, &parseDataP->av.attribute);
    if (answer != "OK")
    {
      return answer;
    }

    if ((answer = parseDataP->av.attribute.check(orionldState.apiVersion, EntityAttributeValueRequest)) != "OK")
    {
      OrionError oe(SccBadRequest, answer);
      return oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);
    }
    break;

  default:
    OrionError oe(SccUnsupportedMediaType, "not supported content type: text/plain");

    answer = oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);

    alarmMgr.badInput(orionldState.clientIp, "not supported content type: text/plain");
    break;
  }

  return answer;
}


