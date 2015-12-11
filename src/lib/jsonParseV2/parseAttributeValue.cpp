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

#include "alarmMgr/alarmMgr.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseAttributeValue.h"
#include "jsonParseV2/parseContextAttribute.h"
#include "jsonParseV2/parseContextAttributeCompoundValue.h"

using namespace rapidjson;



/* ****************************************************************************
*
* parseAttributeValue - 
*/
std::string parseAttributeValue(ConnectionInfo* ciP, ContextAttribute* caP)
{
  Document   document;
  OrionError oe;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    alarmMgr.badInput(clientIp, "JSON parse error");
    oe.fill(SccBadRequest, "Errors found in incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;;
    return oe.render(ciP, "");
  }


  if (!document.IsObject())
  {
    alarmMgr.badInput(clientIp, "JSON parse error");
    oe.fill(SccBadRequest, "Error parsing incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;;
    return oe.render(ciP, "");
  }


  if (!document.HasMember("value"))
  {
    alarmMgr.badInput(clientIp, "No attribute value specified");
    oe.fill(SccBadRequest, "no attribute value specified");
    ciP->httpStatusCode = SccBadRequest;;
    return oe.render(ciP, "");
  }


  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name != "value")
    {
      alarmMgr.badInput(clientIp, "unexpected JSON field - accepting only 'value'");
      oe.fill(SccBadRequest, "unexpected JSON field - accepting only /value/");
      ciP->httpStatusCode = SccBadRequest;;
      return oe.render(ciP, "");
    }


    if (type == "String")
    {
      caP->valueType   = orion::ValueTypeString;
      caP->stringValue = iter->value.GetString();
    }
    else if (type == "Number")
    {
      caP->numberValue  = iter->value.GetDouble();
      caP->valueType    = orion::ValueTypeNumber;
    }
    else if (type == "True")
    {
      caP->boolValue    = true;
      caP->valueType    = orion::ValueTypeBoolean;
    }
    else if (type == "False")
    {
      caP->boolValue    = false;
      caP->valueType    = orion::ValueTypeBoolean;
    }
    else if (type == "Array")
    {
      caP->valueType = orion::ValueTypeVector;

      std::string r = parseContextAttributeCompoundValue(iter, caP, NULL);
      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, "json error in ContextAttributeObject::Vector");
        return "json error in ContextAttributeObject::Vector";
      }
    }
    else if (type == "Object")
    {
      caP->valueType = orion::ValueTypeObject;

      std::string r = parseContextAttributeCompoundValue(iter, caP, NULL);
      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, "json error in ContextAttributeObject::Object");
        return "json error in ContextAttributeObject::Object";
      }
    }
  }

  return "OK";
}
