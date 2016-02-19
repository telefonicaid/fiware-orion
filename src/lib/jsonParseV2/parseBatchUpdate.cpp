/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "rapidjson/document.h"

#include "alarmMgr/alarmMgr.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseEntityVector.h"
#include "jsonParseV2/parseAttributeList.h"
#include "jsonParseV2/parseScopeVector.h"

using namespace rapidjson;



/* ****************************************************************************
*
* parseBatchUpdate - 
*/
std::string parseBatchUpdate(ConnectionInfo* ciP, BatchUpdate* burP)
{
  Document document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    ErrorCode ec;

    alarmMgr.badInput(clientIp, "JSON Parse Error");
    ec.fill(ERROR_STRING_PARSERROR, "Errors found in incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;

    return ec.toJson(true);
  }

  if (!document.IsObject())
  {
    ErrorCode ec;

    alarmMgr.badInput(clientIp, "JSON Parse Error");
    ec.fill("BadRequest", "JSON Parse Error");
    ciP->httpStatusCode = SccBadRequest;

    return ec.toJson(true);
  }
  else if (document.ObjectEmpty())
  {
    ErrorCode ec;

    alarmMgr.badInput(clientIp, "Empty JSON payload");
    ec.fill("BadRequest", "empty payload");
    ciP->httpStatusCode = SccBadRequest;

    return ec.toJson(true);
  }
  else if (!document.HasMember("entities"))
  {
    ErrorCode    ec;
    std::string  details = "Invalid JSON payload, mandatory field /entities/ not found";

    alarmMgr.badInput(clientIp, details);
    ec.fill("BadRequest", details);
    ciP->httpStatusCode = SccBadRequest;

    return ec.toJson(true);
  }
  else if (!document.HasMember("actionType"))
  {
    ErrorCode    ec;
    std::string  details = "Invalid JSON payload, mandatory field /actionType/ not found";

    alarmMgr.badInput(clientIp, details);
    ec.fill("BadRequest", details);
    ciP->httpStatusCode = SccBadRequest;

    return ec.toJson(true);
  }

  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "entities")
    {
      std::string r = parseEntityVector(ciP, iter, &burP->entities, true); // param 4: attributes are allowed in payload

      if (r != "OK")
      {
        ErrorCode ec("BadRequest", r);

        alarmMgr.badInput(clientIp, r);
        ciP->httpStatusCode = SccBadRequest;
        return ec.toJson(true);
      }
    }
    else if (name == "actionType")
    {
      burP->updateActionType.set(iter->value.GetString());
    }
    else
    {
      std::string  description = std::string("Unrecognized field in JSON payload: /") + name + "/";
      ErrorCode    ec("BadRequest", description);

      alarmMgr.badInput(clientIp, description);
      ciP->httpStatusCode = SccBadRequest;

      return ec.toJson(true);
    }
  }

  return "OK";
}
