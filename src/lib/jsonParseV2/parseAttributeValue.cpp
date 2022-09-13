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

#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "ngsi/ParseData.h"


#include "jsonParseV2/parseAttributeValue.h"
#include "jsonParseV2/parseCompoundCommon.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/utilsParse.h"



/* ****************************************************************************
*
* parseContextAttributeCompoundValue -
*/
static std::string parseContextAttributeCompoundValue
(
  rapidjson::Document&  document,
  ContextAttribute*     caP
)
{
  caP->compoundValueP            = new orion::CompoundValueNode();
  caP->compoundValueP->name      = "";
  caP->compoundValueP->valueType = caP->valueType;  // Convert to other type?

  orion::CompoundValueNode*   parent  = caP->compoundValueP;

  //
  // Children of the node
  //
  if (caP->valueType == orion::ValueTypeVector)
  {
    for (rapidjson::Value::ConstValueIterator iter = document.Begin(); iter != document.End(); ++iter)
    {
      std::string                nodeType  = jsonParseTypeNames[iter->GetType()];
      orion::CompoundValueNode*  cvnP      = new orion::CompoundValueNode();

      cvnP->valueType  = stringToCompoundType(nodeType);

      if (nodeType == "String")
      {
        cvnP->stringValue = iter->GetString();
      }
      else if (nodeType == "Number")
      {
        cvnP->numberValue = iter->GetDouble();
      }
      else if ((nodeType == "True") || (nodeType == "False"))
      {
        cvnP->boolValue   = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Null")
      {
        cvnP->valueType = orion::ValueTypeNull;
      }
      else if (nodeType == "Object")
      {
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Start recursive calls if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        std::string r = parseCompoundValue(iter, cvnP, 0);
        if (r != "OK")
        {
          // Early return
          return r;
        }
      }
      else if (!caP->typeGiven)
      {
        caP->type = defaultType(caP->valueType);
      }
    }
  }
  else if (caP->valueType == orion::ValueTypeObject)
  {
    for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
    {
      std::string                nodeType = jsonParseTypeNames[iter->value.GetType()];
      orion::CompoundValueNode*  cvnP     = new orion::CompoundValueNode();

      cvnP->name       = iter->name.GetString();
      cvnP->valueType  = stringToCompoundType(nodeType);

      if (nodeType == "String")
      {
        cvnP->stringValue = iter->value.GetString();
      }
      else if (nodeType == "Number")
      {
        cvnP->numberValue = iter->value.GetDouble();
      }
      else if ((nodeType == "True") || (nodeType == "False"))
      {
        cvnP->boolValue   = (nodeType == "True")? true : false;
      }
      else if (nodeType == "Null")
      {
        cvnP->valueType = orion::ValueTypeNull;
      }
      else if (nodeType == "Object")
      {
        cvnP->valueType = orion::ValueTypeObject;
      }
      else if (nodeType == "Array")
      {
        cvnP->valueType = orion::ValueTypeVector;
      }

      parent->childV.push_back(cvnP);

      //
      // Start recursive calls if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        std::string r = parseCompoundValue(iter, cvnP, 0);
        if (r != "OK")
        {
          // Early return
          return r;
        }
      }
      else if (!caP->typeGiven)
      {
        caP->type = defaultType(caP->valueType);
      }
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* parseAttributeValue - 
*/
std::string parseAttributeValue(ConnectionInfo* ciP, ContextAttribute* caP)
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


  if (!document.IsObject() && !document.IsArray())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error", "Neither JSON Object nor JSON Array for attribute::value");
    oe.fill(SccBadRequest, "Neither JSON Object nor JSON Array for attribute::value", ERROR_BAD_REQUEST);
    ciP->httpStatusCode = SccBadRequest;
    return oe.toJson();
  }

  caP->valueType  = (document.IsObject())? orion::ValueTypeObject : orion::ValueTypeVector;
  std::string r   = parseContextAttributeCompoundValue(document, caP);

  if (r == "max deep reached")
  {
    OrionError oe(SccBadRequest, ERROR_DESC_PARSE_MAX_JSON_NESTING, ERROR_PARSE);
    alarmMgr.badInput(clientIp, r);
    ciP->httpStatusCode = SccBadRequest;
    return oe.toJson();
  }
  else if (r != "OK")  // other error cases get a general treatment
  {
    OrionError oe(SccBadRequest, r, ERROR_BAD_REQUEST);
    alarmMgr.badInput(clientIp, r);
    ciP->httpStatusCode = SccBadRequest;
    return oe.toJson();
  }

  return "OK";
}
