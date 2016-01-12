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
  if (ciP->payload[0] == '"')
  {
    char* end = &ciP->payload[strlen(ciP->payload) - 1];

    if (*end == '"')
    {
      *end = 0;
      caP->stringValue = &ciP->payload[1];
      caP->valueType   = orion::ValueTypeString;
    }
    else
    {
      OrionError oe(SccBadRequest, "Missing citation-mark at end of string");

      return oe.render(ciP, "");
    }
  }

  // 2. True or false?
  else if ((strlen(ciP->payload) == 4) && ((strcmp(ciP->payload, "true") == 0) || (strcmp(ciP->payload, "True") == 0) || (strcmp(ciP->payload, "TRUE") == 0)))
  {
    caP->boolValue   = true;
    caP->valueType   = orion::ValueTypeBoolean;
  }
  else if ((strlen(ciP->payload) == 5) && ((strcmp(ciP->payload, "false") == 0) || (strcmp(ciP->payload, "False") == 0) || (strcmp(ciP->payload, "FALSE") == 0)))
  {
    caP->boolValue   = false;
    caP->valueType   = orion::ValueTypeBoolean;
  }

  // 3. Null ?
  else if ((strlen(ciP->payload) == 4) && ((strcmp(ciP->payload, "null") == 0) || (strcmp(ciP->payload, "Null") == 0) || (strcmp(ciP->payload, "NULL") == 0)))
  {
    caP->valueType   = orion::ValueTypeNone;
  }

  //
  // 4. Valid Double?
  // FIXME P4: this is much more complex than just (atof(string) != 0 || string == "0")
  //           0.000 also is a valid float and it given 0 - 0.0000000000, and 0e0 also ...
  //           And, even worse, 123K gives 123.0 back, we would need to analyze the string to try to
  //           find garbage bytes after it if we want to detect this error.
  //           However, all of this is not so extremely important and for now, (strtod(string) != 0 || string== "0") is good enough
  //
  else if (((d = strtod(ciP->payload, NULL)) != 0) || ((ciP->payload[0] == '0') && (ciP->payload[1] == 0)))
  {
    caP->valueType   = orion::ValueTypeNumber;
    caP->numberValue = d;
  }

  // 5. None of the above - it's a string
  else
  {
    caP->stringValue = ciP->payload;
    caP->valueType   = orion::ValueTypeString;
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

    if ((answer = parseDataP->av.attribute.check(EntityAttributeValueRequest, ciP->outFormat, "", "", 0)) != "OK")
    {
      OrionError error(SccBadRequest, answer);
      return error.render(ciP, "");
    }
    break;

  default:
    OrionError error(SccUnsupportedMediaType, "not supported content type: text/plain");

    answer = error.render(ciP, "");
    ciP->httpStatusCode = SccUnsupportedMediaType;

    alarmMgr.badInput(clientIp, "not supported content type: text/plain");
    break;
  }
  
  return answer;
}


