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
* parseBatchQuery - 
*/
std::string parseBatchQuery(ConnectionInfo* ciP, BatchQuery* bqrP)
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
  else if (!document.HasMember("entities") && !document.HasMember("attributes") && !document.HasMember("scopes"))
  {
    ErrorCode ec;

    alarmMgr.badInput(clientIp, "Invalid JSON payload, no relevant fields found");
    ec.fill("BadRequest", "Invalid JSON payload, no relevant fields found");
    ciP->httpStatusCode = SccBadRequest;

    return ec.toJson(true);
  }

  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "entities")
    {
      std::string r = parseEntityVector(ciP, iter, &bqrP->entities);

      if (r != "OK")
      {
        ErrorCode ec("BadRequest", r);

        alarmMgr.badInput(clientIp, r);
        ciP->httpStatusCode = SccBadRequest;
        return ec.toJson(true);
      }
    }
    else if (name == "attributes")
    {
      std::string r = parseAttributeList(ciP, iter, &bqrP->attributeV);

      if (r != "OK")
      {
        ErrorCode ec("BadRequest", r);

        alarmMgr.badInput(clientIp, r);
        ciP->httpStatusCode = SccBadRequest;
        return ec.toJson(true);
      }
    }
    else if (name == "scopes")
    {
      std::string r = parseScopeVector(ciP, iter, &bqrP->scopeV);

      if (r != "OK")
      {
        ErrorCode ec("BadRequest", r);

        alarmMgr.badInput(clientIp, r);
        ciP->httpStatusCode = SccBadRequest;
        return ec.toJson(true);
      }
    }
    else
    {
      std::string  description = std::string("Unrecognizedfield in JSON payload: /") + name + "/";
      ErrorCode    ec("BadRequest", description);

      alarmMgr.badInput(clientIp, description);
      ciP->httpStatusCode = SccBadRequest;

      return ec.toJson(true);
    }
  }

  return "OK";
}
