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

#include "rapidjson/document.h"

#include "common/globals.h"
#include "common/errorMessages.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "parse/forbiddenChars.h"
#include "alarmMgr/alarmMgr.h"

#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseContextAttribute.h"
#include "jsonParseV2/parseEntity.h"



/* ****************************************************************************
*
* parseEntity -
*
* This function is used to parse two slightly different payloads:
* - POST /v2/entities
* - POST /v2/entities/<eid>
*
* In the latter case, "id" CANNOT be in the payload, while in the former case,
* "id" MUST be in the payload.
*
* In the case of /v2/entities/<eid>, the entityId of 'Entity* eP' is set in
* the service routine postEntity.
*
* Also, if the URI param 'options' includes the value 'keyValues', then the
* parse changes for compound values of attributes. If the value is a JSON object
* then there is no looking inside to find the 'value' field, but the attribute is
* always treated as a compound attribute.
*/
std::string parseEntity(ConnectionInfo* ciP, Entity* eP, bool eidInURL)
{
  rapidjson::Document document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    OrionError oe(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);

    alarmMgr.badInput(clientIp, "JSON parse error");
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }


  if (!document.IsObject())
  {
    OrionError oe(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);

    alarmMgr.badInput(clientIp, "JSON Parse Error");
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }


  if (eidInURL == false)
  {
    if (!document.HasMember("id"))
    {
      OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_ID, ERROR_BAD_REQUEST);

      alarmMgr.badInput(clientIp, "No entity id specified");
      ciP->httpStatusCode = SccBadRequest;

      return oe.toJson();
    }
  }


  if (eidInURL == true)
  {
    if (document.HasMember("id"))
    {
      OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_ENTID_IN_PAYLOAD, "BadRequest");

      alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_ENTID_IN_PAYLOAD);
      ciP->httpStatusCode = SccBadRequest;

      return oe.toJson();
    }

    if (document.HasMember("type"))
    {
      OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_ENTTYPE_IN_PAYLOAD, "BadRequest");

      alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_ENTTYPE_IN_PAYLOAD);
      ciP->httpStatusCode = SccBadRequest;

      return oe.toJson();
    }
  }
  else if (document.ObjectEmpty())
  {
    //
    // Initially we used the method "Empty". As the broker crashed inside that method, some
    // research was made and "ObjectEmpty" was found. As the broker stopped crashing and complaints
    // about crashes with small docs and "Empty()" were found on the internet, we opted to use ObjectEmpty
    //
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD, ERROR_BAD_REQUEST);

    alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  int membersFound = 0;
  for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    ++membersFound;

    if (name == "id")
    {
      if (eidInURL == false)
      {
        if (type != "String")
        {
          OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTID, "BadRequest");

          alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTID);
          ciP->httpStatusCode = SccBadRequest;

          return oe.toJson();
        }

        eP->id = iter->value.GetString();

        if (forbiddenIdChars(ciP->apiVersion, eP->id.c_str(), ""))
        {
          OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID, "BadRequest");

          alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID);
          ciP->httpStatusCode = SccBadRequest;

          return oe.toJson();
        }
      }
      else  // "id" is present in payload for /v2/entities/<eid> - not a valid payload
      {
        OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_ID_AS_ATTR, "BadRequest");

        alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_ID_AS_ATTR);
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
    }
    else if (name == "type")
    {
      if (type != "String")
      {
        OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTTYPE, "BadRequest");

        alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTTYPE);
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }

      eP->type      = iter->value.GetString();
      eP->typeGiven = true;

      if (eP->type.empty())
      {
        const char* errorText = ERROR_DESC_BAD_REQUEST_EMPTY_ENTTYPE;
        OrionError  oe(SccBadRequest, errorText, "BadRequest");

        alarmMgr.badInput(clientIp, errorText);
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }

      if (forbiddenIdChars(ciP->apiVersion, eP->type.c_str(), ""))
      {
        OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE, "BadRequest");

        alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE);
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
    }
    else  // attribute
    {
      ContextAttribute* caP = new ContextAttribute();

      eP->attributeVector.push_back(caP);

      std::string r = parseContextAttribute(ciP, iter, caP);
      if (r == "max deep reached")
      {
        OrionError oe(SccBadRequest, ERROR_DESC_PARSE_MAX_JSON_NESTING, ERROR_PARSE);

        alarmMgr.badInput(clientIp, "max json deep reached at parsing");
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
      else if (r != "OK")  // other error cases get a general treatment
      {
        OrionError oe(SccBadRequest, r, "BadRequest");

        alarmMgr.badInput(clientIp, "parse error in context attribute");
        ciP->httpStatusCode = SccBadRequest;

        return oe.toJson();
      }
    }
  }

  if (membersFound == 0)
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD, ERROR_BAD_REQUEST);

    alarmMgr.badInput(clientIp, "empty payload");
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  if (eidInURL == false)
  {
    if (eP->id.empty())
    {
      OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_ID, ERROR_BAD_REQUEST);

      alarmMgr.badInput(clientIp, "empty entity id");
      ciP->httpStatusCode = SccBadRequest;

      return oe.toJson();
    }
  }

  if (!eP->typeGiven)
  {
    eP->type = DEFAULT_ENTITY_TYPE;
  }

  return "OK";
}
