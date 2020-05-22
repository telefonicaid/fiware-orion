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
#include <string>

#include "rapidjson/document.h"

#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "jsonParseV2/parseEntityVector.h"
#include "jsonParseV2/parseStringList.h"
#include "jsonParseV2/parseBatchQuery.h"
#include "jsonParseV2/parseExpression.h"



/* ****************************************************************************
*
* parseBatchQuery -
*/
std::string parseBatchQuery(ConnectionInfo* ciP, BatchQuery* bqrP)
{
  rapidjson::Document    document;
  OrionError             oe;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error");
    oe.fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  if (!document.IsObject())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error");
    oe.fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }
  else if (document.ObjectEmpty())
  {
    alarmMgr.badInput(clientIp, "Empty JSON payload");
    oe.fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }
  else if (!document.HasMember("entities") && !document.HasMember("attributes")
           && !document.HasMember("attrs") && !document.HasMember("expression"))
  {
    alarmMgr.badInput(clientIp, "Invalid JSON payload, no relevant fields found");
    oe.fill(SccBadRequest, "Invalid JSON payload, no relevant fields found", "BadRequest");
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();

    if (name == "entities")
    {
      // param 4 to parseEntityVector(): attributes are NOT allowed in payload
      std::string r = parseEntityVector(ciP, iter, &bqrP->entities, true, false);

      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, r);
        oe.fill(SccBadRequest, r, "BadRequest");
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
    }
    // This is deprecated: use "expression" "q" unary operator and "attrs" instead
    else if (name == "attributes")
    {
      std::string r = parseStringList(ciP, iter, &bqrP->attributeV, name);

      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, r);
        oe.fill(SccBadRequest, r, "BadRequest");
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
    }
    else if (name == "attrs")
    {
      std::string r = parseStringList(ciP, iter, &bqrP->attrsV, name, true);

      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, r);
        oe.fill(SccBadRequest, r, "BadRequest");
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
    }
    else if (name == "expression")
    {
      std::string r = parseExpression(iter->value, &bqrP->scopeV, NULL);

      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, r);
        oe.fill(SccBadRequest, r, "BadRequest");
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
    }
    else if (name == "metadata")
    {
      std::string r = parseStringList(ciP, iter, &bqrP->metadataV, name, true);

      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, r);
        oe.fill(SccBadRequest, r, "BadRequest");
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
    }
    else
    {
      std::string  description = std::string("Unrecognized field in JSON payload: /") + name + "/";

      alarmMgr.badInput(clientIp, description);
      oe.fill(SccBadRequest, description, "BadRequest");
      ciP->httpStatusCode = SccBadRequest;

      return oe.toJson();
    }
  }

  return "OK";
}
