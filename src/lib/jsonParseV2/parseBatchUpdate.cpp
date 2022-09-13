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
#include "jsonParseV2/parseBatchUpdate.h"
#include "jsonParseV2/utilsParse.h"
#include "orionTypes/UpdateActionType.h"



/* ****************************************************************************
*
* parseBatchUpdate -
*/
std::string parseBatchUpdate(ConnectionInfo* ciP, BatchUpdate* burP)
{
  rapidjson::Document  document;
  OrionError           oe;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error", parseErrorString(document.GetParseError()));
    oe.fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  if (!document.IsObject())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error", "JSON Object not found");
    oe.fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }
  else if (document.ObjectEmpty())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error", "Empty JSON payload");
    oe.fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }
  else if (!document.HasMember("entities"))
  {
    std::string  details = "Invalid JSON payload, mandatory field /entities/ not found";

    alarmMgr.badInput(clientIp, "JSON Parse Error", details);
    oe.fill(SccBadRequest, details, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }
  else if (!document.HasMember("actionType"))
  {
    std::string  details = "Invalid JSON payload, mandatory field /actionType/ not found";

    alarmMgr.badInput(clientIp, details);
    oe.fill(SccBadRequest, details, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();

    if (name == "entities")
    {
      // param 4 to parseEntityVector(): idPattern is not allowed in batch updates
      // param 5 for parseEntityVector(): attributes are allowed in payload
      std::string r = parseEntityVector(ciP, iter, &burP->entities, false, true);

      if (r == "NotImplemented")
      {
        alarmMgr.badInput(clientIp, r);
        oe.fill(SccNotImplemented, r, "NotImplemented");
        ciP->httpStatusCode = SccNotImplemented;
        r = oe.toJson();
        return r;
      }
      else if (r != "OK")
      {
        alarmMgr.badInput(clientIp, r);
        oe.fill(SccBadRequest, r, ERROR_BAD_REQUEST);
        ciP->httpStatusCode = SccBadRequest;
        r = oe.toJson();
        return r;
      }
    }
    else if (name == "actionType")
    {
      ActionType   actionType;
      std::string  actionTypeStr = iter->value.GetString();
      if ((actionType = parseActionTypeV2(actionTypeStr)) == ActionTypeUnknown)
      {
        std::string details = "invalid update action type: right ones are: "
          "append, appendStric, delete, replace, update";

        alarmMgr.badInput(clientIp, details, actionTypeStr);
        oe.fill(SccBadRequest, details, ERROR_BAD_REQUEST);
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
      burP->updateActionType = actionType;
    }
    else
    {
      std::string  description = std::string("Unrecognized field in JSON payload: /") + name + "/";

      alarmMgr.badInput(clientIp, description);
      oe.fill(SccBadRequest, description, ERROR_BAD_REQUEST);
      ciP->httpStatusCode = SccBadRequest;

      return oe.toJson();
    }
  }

  return "OK";
}
