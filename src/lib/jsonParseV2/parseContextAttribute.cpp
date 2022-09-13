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

#include "logMsg/logMsg.h"

#include "common/errorMessages.h"
#include "ngsi/ContextAttribute.h"
#include "parse/CompoundValueNode.h"
#include "parse/forbiddenChars.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"

#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseMetadataVector.h"
#include "jsonParseV2/parseContextAttribute.h"
#include "jsonParseV2/parseCompoundCommon.h"
#include "jsonParseV2/utilsParse.h"



/* ****************************************************************************
*
* checkGeoJson -
*
* Check that attribute is null or an object. Other checks for Feature and FeatureCollection
* are done in location.cpp functions, as they has to be done only when ignoreType:true
* is not in the entity
*
*/
static std::string checkGeoJson(ContextAttribute* caP)
{
  if (caP->compoundValueP == NULL)
  {
    // In no object or vector, only null is allowed
    if (caP->valueType == orion::ValueTypeNull)
    {
      return "OK";
    }
    else
    {
      return "geo:json needs an object or null as value";
    }
  }

  // Vector not allowed
  if (!caP->compoundValueP->isObject())
  {
    return "geo:json needs an object or null as value";
  }

  return "OK";
}



/* ****************************************************************************
*
* prepareContextAttributeCompoundRoot -
*/
static void prepareContextAttributeCompoundRoot(ContextAttribute* caP, const std::string& type)
{
  caP->compoundValueP            = new orion::CompoundValueNode();
  caP->compoundValueP->name      = "";
  caP->compoundValueP->valueType = stringToCompoundType(type);

  if (!caP->typeGiven)
  {
     caP->type = (type == "Object")? defaultType(orion::ValueTypeObject) : defaultType(orion::ValueTypeVector);
  }
}



/* ****************************************************************************
*
* parseContextAttributeObject -
*/
static std::string parseContextAttributeObject
(
  const rapidjson::Value&  start,
  ContextAttribute*        caP,
  bool*                    compoundVector
)
{
  // This is NGSIv2 parsing and in NGSIv2, no value means implicit null. Note that
  // valueTypeNotGiven will be overridden inside the 'for' block in case the attribute has an actual value
  caP->valueType = orion::ValueTypeNull;

  // It may happen in the for iterator to see the same key twice. This is a problem for "value" in the
  // case of compounds and may lead to a crash (see details in issue #3603). Thus, we need to control
  // explicitely value has been set with this flag
  bool valueSet = false;

  for (rapidjson::Value::ConstMemberIterator iter = start.MemberBegin(); iter != start.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "type")
    {
      if (type != "String")
      {
        return "invalid JSON type for attribute type";
      }

      caP->type      = iter->value.GetString();
      caP->typeGiven = true;
    }
    else if (name == "value")
    {
      if (valueSet)
      {
        return "duplicated value key in attribute";
      }
      valueSet = true;

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
        caP->valueType    = orion::ValueTypeNull;
      }
      else if (type == "Array")
      {
        //
        // FIXME P4: Here the attribute's valueType is set to ValueTypeVector, but normally all compounds have the
        //           valueType set as Object in the attribute ...
        //           to later find its real type in compoundValueP->valueType.
        //           This seems to be needed later, so no 'fix' for now.
        //           However, this should be looked into, and probably the attributes with compound values
        //           should have the real type of its compound (Object|Vector), not always Object.
        //           This is really confusing ...
        //           I guess this was to be able to easily compare for compound by checking
        //           "caP->valueType == orion::ValueTypeObject",
        //           but it is equally easy to compare "caP->compoundValueP != NULL" instead.
        //
        caP->valueType  = orion::ValueTypeVector;
        *compoundVector = true;
        prepareContextAttributeCompoundRoot(caP, jsonParseTypeNames[iter->value.GetType()]);
        std::string r   = parseCompoundValue(iter, caP->compoundValueP, 0);
        if (r != "OK")
        {
          return r;
        }
      }
      else if (type == "Object")
      {
        caP->valueType  = orion::ValueTypeObject;
        prepareContextAttributeCompoundRoot(caP, jsonParseTypeNames[iter->value.GetType()]);
        std::string r   = parseCompoundValue(iter, caP->compoundValueP, 0);
        if (r != "OK")
        {
          return r;
        }
      }
    }
    else if (name == "metadata")
    {
      std::string r  = parseMetadataVector(iter, caP);

      if (r != "OK")
      {
        return r;
      }
    }
    else  // ERROR
    {
      LM_W(("Bad Input (unrecognized property for ContextAttribute - '%s')", name.c_str()));
      return "unrecognized property for context attribute";
    }
  }

  // Is it a (not null) date?
  if (((caP->type == DATE_TYPE) || (caP->type == DATE_TYPE_ALT)) && (caP->valueType != orion::ValueTypeNull))
  {
    caP->numberValue =  parse8601Time(caP->stringValue);

    if (caP->numberValue == -1)
    {
      return "date has invalid format";
    }

    // Probably reseting stringValue is not needed, but let's do it for cleanliness
    caP->stringValue = "";
    caP->valueType   = orion::ValueTypeNumber;
  }

  // It is a safe GeoJSON?
  if (caP->type == GEO_JSON)
  {
    std::string r = checkGeoJson(caP);
    if (r != "OK")
    {
      return r;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* parseContextAttribute -
*/
std::string parseContextAttribute
(
  ConnectionInfo*                               ciP,
  const rapidjson::Value::ConstMemberIterator&  iter,
  ContextAttribute*                             caP
)
{
  std::string  name           = iter->name.GetString();
  std::string  type           = jsonParseTypeNames[iter->value.GetType()];
  bool         keyValues      = ciP->uriParamOptions[OPT_KEY_VALUES];
  bool         compoundVector = false;

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
      caP->valueType   = orion::ValueTypeNull;
    }
    else if (type == "Array")
    {
      compoundVector  = true;
      caP->valueType  = orion::ValueTypeObject;
      prepareContextAttributeCompoundRoot(caP, jsonParseTypeNames[iter->value.GetType()]);
      std::string r   = parseCompoundValue(iter, caP->compoundValueP, 0);

      if (r == "max deep reached")
      {
        alarmMgr.badInput(clientIp, "max deep reached", "found in ContextAttributeObject::Vector");
        ciP->httpStatusCode = SccBadRequest;
        return "max deep reached";
      }
      else if (r != "OK")  // other error cases get a general treatment
      {
        alarmMgr.badInput(clientIp, "JSON Parse Error in ContextAttribute::Vector", r);
        ciP->httpStatusCode = SccBadRequest;
        return "json error in ContextAttribute::Vector";
      }
    }
    else if (type == "Object")
    {
      caP->valueType = orion::ValueTypeObject;
      prepareContextAttributeCompoundRoot(caP, jsonParseTypeNames[iter->value.GetType()]);
      std::string r = parseCompoundValue(iter, caP->compoundValueP, 0);

      if (r == "max deep reached")
      {
        alarmMgr.badInput(clientIp, "max deep reached", "found in ContextAttributeObject::Object");
        ciP->httpStatusCode = SccBadRequest;
        return "max deep reached";
      }
      else if (r != "OK")  // other error cases get a general treatment
      {
        alarmMgr.badInput(clientIp, "JSON Parse Error in ContextAttribute::Object", r);
        ciP->httpStatusCode = SccBadRequest;
        return "json error in ContextAttribute::Object";
      }
    }
    else
    {
      alarmMgr.badInput(clientIp, "JSON Parse Error", "bad JSON type for ContextAttribute");
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
      std::string details = "attribute must be a JSON object, unless keyValues option is used";
      alarmMgr.badInput(clientIp, "JSON Parse Error", details);
      ciP->httpStatusCode = SccBadRequest;
      return details;
    }

    // Attribute has a regular structure, in which 'value' is mandatory (except in v2)
    if (iter->value.HasMember("value") || ciP->apiVersion == V2)
    {
      std::string r = parseContextAttributeObject(iter->value, caP, &compoundVector);
      if (r == "max deep reached")
      {
        alarmMgr.badInput(clientIp, "max deep reached", "found in ContextAttributeObject::Object");
        ciP->httpStatusCode = SccBadRequest;
        return "max deep reached";
      }
      else if (r != "OK")  // other error cases get a general treatment
      {
        alarmMgr.badInput(clientIp, "JSON Parse Error in ContextAttribute::Object", r);
        ciP->httpStatusCode = SccBadRequest;
        return r;
      }
    }
    else
    {
      alarmMgr.badInput(clientIp, "JSON Parse Error", "no 'value' for ContextAttribute without keyValues");
      ciP->httpStatusCode = SccBadRequest;
      return "no 'value' for ContextAttribute without keyValues";
    }
  }

  if (!caP->typeGiven)
  {
    caP->type = (compoundVector)? defaultType(orion::ValueTypeVector) : defaultType(caP->valueType);
  }

  std::string r = caP->check(ciP->apiVersion, ciP->requestType);
  if (r != "OK")
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error", r);
    ciP->httpStatusCode = SccBadRequest;
    return r;
  }

  return "OK";
}



/* ****************************************************************************
*
* parseContextAttribute -
*/
std::string parseContextAttribute(ConnectionInfo* ciP, ContextAttribute* caP)
{
  rapidjson::Document  document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    OrionError oe(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);

    alarmMgr.badInput(clientIp, "JSON Parse Error", parseErrorString(document.GetParseError()));
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }


  if (!document.IsObject())
  {
    OrionError oe(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);

    alarmMgr.badInput(clientIp, "JSON Parse Error", "JSON Object not found");
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  bool         compoundVector = false;
  std::string  r = parseContextAttributeObject(document, caP, &compoundVector);

  if (r == "max deep reached")
  {
    OrionError oe(SccBadRequest, ERROR_DESC_PARSE_MAX_JSON_NESTING, ERROR_PARSE);

    alarmMgr.badInput(clientIp, "max deep reached", r);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }
  else if (r != "OK")  // other error cases get a general treatment
  {
    OrionError oe(SccBadRequest, r, ERROR_BAD_REQUEST);

    alarmMgr.badInput(clientIp, "JSON Parse Error", r);
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  if (!caP->typeGiven)
  {
    caP->type = (compoundVector)? defaultType(orion::ValueTypeVector) : defaultType(caP->valueType);
  }

  return r;
}
