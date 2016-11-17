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
#include "rapidjson/document.h"

#include "common/globals.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseEntity.h"
#include "jsonParseV2/parseContextAttribute.h"
#include "alarmMgr/alarmMgr.h"

using namespace rapidjson;



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
* 
*/
std::string parseEntity(ConnectionInfo* ciP, Entity* eP, bool eidInURL)
{
  Document document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    alarmMgr.badInput(clientIp, "JSON parse error");
    ciP->httpStatusCode = SccBadRequest;
    OrionError oe(SccBadRequest, "Errors found in incoming JSON buffer", ERROR_STRING_PARSERROR);
    return oe.toJson();
  }


  if (!document.IsObject())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error");
    ciP->httpStatusCode = SccBadRequest;
    OrionError oe(SccBadRequest, "Errors found in incoming JSON buffer", ERROR_STRING_PARSERROR);
    return oe.toJson();
  }


  if (eidInURL == false)
  {
    if (!document.HasMember("id"))
    {
      alarmMgr.badInput(clientIp, "No entity id specified");
      ciP->httpStatusCode = SccBadRequest;
      OrionError oe(SccBadRequest, "no entity id specified", "BadRequest");
      return oe.toJson();
    }
  }


  if (eidInURL == true)
  {
    if (document.HasMember("id"))
    {
      alarmMgr.badInput(clientIp, "entity id specified in payload");
      ciP->httpStatusCode = SccBadRequest;
      OrionError oe(SccBadRequest, "entity id specified in payload", "BadRequest");
      return oe.toJson();
    }

    if (document.HasMember("type"))
    {
      alarmMgr.badInput(clientIp, "entity type specified in payload");
      ciP->httpStatusCode = SccBadRequest;
      OrionError oe(SccBadRequest, "entity type specified in payload", "BadRequest");
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
    alarmMgr.badInput(clientIp, "Empty payload");
    ciP->httpStatusCode = SccBadRequest;
    OrionError oe(SccBadRequest, "empty payload", "BadRequest");
    return oe.toJson();
  }

  int membersFound = 0;
  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
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
          alarmMgr.badInput(clientIp, "invalid JSON type for entity id");
          ciP->httpStatusCode = SccBadRequest;;
          OrionError oe(SccBadRequest, "invalid input, 'id' as attribute", "BadRequest");
          return oe.toJson();
        }

        eP->id = iter->value.GetString();
      }
      else  // "id" is present in payload for /v2/entities/<eid> - not a valid payload
      {
        alarmMgr.badInput(clientIp, "'id' is not a valid attribute");
        ciP->httpStatusCode = SccBadRequest;;
        OrionError oe(SccBadRequest, "invalid input, 'id' as attribute", "BadRequest");
        return oe.toJson();
      }
    }
    else if (name == "type")
    {
      if (type != "String")
      {
        alarmMgr.badInput(clientIp, "invalid JSON type for entity type");
        ciP->httpStatusCode = SccBadRequest;
        OrionError oe(SccBadRequest, "invalid JSON type for entity type", "BadRequest");
        return oe.toJson();
      }

      eP->type      = iter->value.GetString();
      eP->typeGiven = true;
    }
    else  // attribute
    {
      ContextAttribute* caP = new ContextAttribute();
      
      eP->attributeVector.push_back(caP);

      std::string r = parseContextAttribute(ciP, iter, caP);
      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, "parse error in context attribute");
        ciP->httpStatusCode = SccBadRequest;
        OrionError oe(SccBadRequest, r, "BadRequest");
        return oe.toJson();
      }
    }
  }

  if (membersFound == 0)
  {
    alarmMgr.badInput(clientIp, "empty payload");
    ciP->httpStatusCode = SccBadRequest;
    OrionError oe(SccBadRequest, "empty payload", "BadRequest");
    return oe.toJson();
  }

  if (eidInURL == false)
  {
    if (eP->id == "")
    {
      alarmMgr.badInput(clientIp, "empty entity id");
      ciP->httpStatusCode = SccBadRequest;
      OrionError oe(SccBadRequest, "empty entity id", "BadRequest");
      return oe.toJson();
    }
  }

  if (!eP->typeGiven)
  {
    eP->type = DEFAULT_ENTITY_TYPE;
  }

  return "OK";
}
