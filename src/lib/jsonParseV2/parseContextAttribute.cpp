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

#include "logMsg/logMsg.h"

#include "ngsi/ContextAttribute.h"
#include "parse/CompoundValueNode.h"
#include "alarmMgr/alarmMgr.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseContextAttribute.h"
#include "jsonParseV2/parseMetadataVector.h"
#include "jsonParseV2/parseContextAttributeCompoundValue.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"

using namespace rapidjson;



/* ****************************************************************************
*
* parseContextAttributeObject - 
*/
static std::string parseContextAttributeObject(const Value& start, ContextAttribute* caP)
{
  int members = 0;

  // valueTypeNone will be overridden inside the 'for' block in case the attribute has an actual value
  caP->valueType = orion::ValueTypeNone;

  for (Value::ConstMemberIterator iter = start.MemberBegin(); iter != start.MemberEnd(); ++iter)
  {
    ++members;

    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "type")
    {
      if (type != "String")
      {
        alarmMgr.badInput(clientIp, "ContextAttributeObject::type must be a String");
        return "invalid JSON type for attribute type";
      }

      caP->type      = iter->value.GetString();
      caP->typeGiven = true;
    }
    else if (name == "value")
    {
      if (type == "String")
      {
        caP->stringValue  = iter->value.GetString();
        caP->valueType    = orion::ValueTypeString;
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
      else if (type == "Null")
      {
        caP->valueType    = orion::ValueTypeNone;
      }
      else if (type == "Array")
      {
        caP->valueType    = orion::ValueTypeVector;

        std::string r = parseContextAttributeCompoundValue(iter, caP, NULL);
        if (r != "OK")
        {
          alarmMgr.badInput(clientIp, "json error in ContextAttributeObject::Vector");
          return "json error in ContextAttributeObject::Vector";
        }
      }
      else if (type == "Object")
      {
        caP->valueType    = orion::ValueTypeObject;

        std::string r = parseContextAttributeCompoundValue(iter, caP, NULL);
        if (r != "OK")
        {
          alarmMgr.badInput(clientIp, "json error in ContextAttributeObject::Object");
          return "json error in ContextAttributeObject::Object";
        }
      }
    }
    else if (name == "metadata")
    {
      std::string r  = parseMetadataVector(iter, caP);

      if (r != "OK")
      {
        std::string details = std::string("error parsing Metadata: ") + r;
        alarmMgr.badInput(clientIp, details);
        return r;
      }
    }
    else // ERROR
    {
      LM_W(("Bad Input (unrecognized property for ContextAttribute - '%s')", name.c_str()));
      return "unrecognized property for context attribute";
    }
  }

  if (members == 0)
  {
    caP->valueType = orion::ValueTypeNone;
  }

  // Is it a date?
  if (caP->type == DATE_TYPE)
  {
    caP->numberValue =  parse8601Time(caP->stringValue);

    if (caP->numberValue == -1)
    {
      return "date has invalid format";
    }

    // Probably reseting stringValue is not needed, but let's do it for cleanness
    caP->stringValue = "";
    caP->valueType   = orion::ValueTypeNumber;

  }

  return "OK";
}



/* ****************************************************************************
*
* parseContextAttribute - 
*/
std::string parseContextAttribute(ConnectionInfo* ciP, const Value::ConstMemberIterator& iter, ContextAttribute* caP)
{
  std::string name      = iter->name.GetString();
  std::string type      = jsonParseTypeNames[iter->value.GetType()];
  bool        keyValues = ciP->uriParamOptions["keyValues"];

  caP->name = name;

  if (keyValues)
  {
    if (type == "String")
    {
      caP->type        = "";
      caP->stringValue = iter->value.GetString();
      caP->valueType   = orion::ValueTypeString;
    }
    else if (type == "Number")
    {
      caP->type        = "";
      caP->valueType   = orion::ValueTypeNumber;
      caP->numberValue = iter->value.GetDouble();
    }
    else if (type == "True")
    {
      caP->type        = "";
      caP->valueType   = orion::ValueTypeBoolean;
      caP->boolValue   = true;
    }
    else if (type == "False")
    {
      caP->type        = "";
      caP->valueType   = orion::ValueTypeBoolean;
      caP->boolValue   = false;
    }
    else if (type == "Null")
    {
      caP->type        = "";
      caP->valueType   = orion::ValueTypeNone;
    }
    else if (type == "Array")
    {
      caP->valueType = orion::ValueTypeObject;
      std::string r = parseContextAttributeCompoundValue(iter, caP, NULL);
      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, "json error in ContextAttribute::Vector");
        ciP->httpStatusCode = SccBadRequest;
        return "json error in ContextAttribute::Vector";
      }
    }
    else if (type == "Object")
    {
      parseContextAttributeCompoundValue(iter, caP, NULL);
      caP->valueType = orion::ValueTypeObject;
    }
    else
    {
      alarmMgr.badInput(clientIp, "bad type for ContextAttribute");
      ciP->httpStatusCode = SccBadRequest;
      return "invalid JSON type for ContextAttribute";
    }
  }
  else  // no keyValues
  {
    // First of all, if no keyValues, we must be in a JSON object.
    std::string type   = jsonParseTypeNames[iter->value.GetType()];
    if (type != "Object")
    {
      std::string details = "attribute must be a JSON object";
      alarmMgr.badInput(clientIp, details);
      ciP->httpStatusCode = SccBadRequest;
      return details;
    }

    // Attribute has a regular structure, in which 'value' is mandatory
    if ((iter->value.HasMember("value")))
    {
      std::string r = parseContextAttributeObject(iter->value, caP);
      if (r != "OK")
      {
        alarmMgr.badInput(clientIp, "JSON parse error in ContextAttribute::Object");
        ciP->httpStatusCode = SccBadRequest;
        return r;
      }
    }
    else
    {
      alarmMgr.badInput(clientIp, "no 'value' for ContextAttribute without keyValues");
      ciP->httpStatusCode = SccBadRequest;
      return "no 'value' for ContextAttribute without keyValues";
    }
  }

  if (caP->name == "")
  {
    alarmMgr.badInput(clientIp, "no 'name' for ContextAttribute");
    ciP->httpStatusCode = SccBadRequest;
    return "no 'name' for ContextAttribute";
  }

  return "OK";
}



/* ****************************************************************************
*
* parseContextAttribute - 
*/
std::string parseContextAttribute(ConnectionInfo* ciP, ContextAttribute* caP)
{
  Document  document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    OrionError oe(SccBadRequest, "Errors found in incoming JSON buffer", ERROR_STRING_PARSERROR);

    alarmMgr.badInput(clientIp, "JSON parse error");
    ciP->httpStatusCode = SccBadRequest;

    return oe.render(ciP, "");
  }


  if (!document.IsObject())
  {
    OrionError oe(SccBadRequest, "Error parsing incoming JSON buffer", ERROR_STRING_PARSERROR);

    alarmMgr.badInput(clientIp, "JSON Parse Error");
    ciP->httpStatusCode = SccBadRequest;

    return oe.render(ciP, "");
  }

  std::string  r = parseContextAttributeObject(document, caP);
  if (r != "OK")
  {
    OrionError oe(SccBadRequest, r);

    ciP->httpStatusCode = SccBadRequest;
    return oe.render(ciP, "");
  }

  return r;
}
