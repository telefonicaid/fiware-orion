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
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseContextAttribute.h"
#include "jsonParseV2/parseMetadata.h"
#include "jsonParseV2/parseContextAttributeCompoundValue.h"
#include "rest/ConnectionInfo.h"

using namespace rapidjson;



/* ****************************************************************************
*
* parseContextAttributeObject - 
*/
static std::string parseContextAttributeObject(const Value& start, ContextAttribute* caP)
{
  for (Value::ConstMemberIterator iter = start.MemberBegin(); iter != start.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    LM_M(("parseContextAttributeObject: %s/%s", name.c_str(), type.c_str()));

    if (name == "type")
    {
      if (type != "String")
      {
        LM_E(("Bad Input (ContextAttributeObject::type must be a String"));
        return "Parse Error";
      }

      caP->type = iter->value.GetString();
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
      else if (type == "Array")
      {
        caP->valueType    = orion::ValueTypeVector;

        std::string r = parseContextAttributeCompoundValue(iter, caP, NULL);
        if (r != "OK")
        {
          LM_W(("Bad Input (json error in ContextAttributeObject::Vector"));
          return "json error in ContextAttributeObject::Vector";
        }
      }
      else if (type == "Object")
      {
        caP->valueType    = orion::ValueTypeObject;

        std::string r = parseContextAttributeCompoundValue(iter, caP, NULL);
        if (r != "OK")
        {
          LM_W(("Bad Input (json error in ContextAttributeObject::Object"));
          return "json error in ContextAttributeObject::Object";
        }
      }
    }
    else  // Metadata
    {
      Metadata*   mP = new Metadata();

      mP->name       = iter->name.GetString();
      std::string r  = parseMetadata(iter->value, mP);

      caP->metadataVector.push_back(mP);

      if (r != "OK")
      {
        LM_W(("Bad Input (error parsing Metadata)"));
        return "JSON Parse Error in ContextAttributeObject::Metadata";
      }
      LM_M(("Metadata OK"));
    }
  }

  LM_M(("Done"));
  return "OK";
}



/* ****************************************************************************
*
* parseContextAttribute - 
*/
std::string parseContextAttribute(ConnectionInfo* ciP, const Value::ConstMemberIterator& iter, ContextAttribute* caP)
{
  LM_M(("KZ: In parseContextAttribute"));

  std::string name   = iter->name.GetString();
  std::string type   = jsonParseTypeNames[iter->value.GetType()];
  
  caP->name = name;

  LM_M(("KZ: name: %s", caP->name.c_str()));

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
  else if (type == "Array")
  {
    LM_M(("KZ: Compound array"));
    caP->valueType = orion::ValueTypeObject;
    std::string r = parseContextAttributeCompoundValue(iter, caP, NULL);
    if (r != "OK")
    {
      LM_W(("Bad Input (json error in ContextAttribute::Vector"));
      ciP->httpStatusCode = SccBadRequest;
      return "json error in ContextAttribute::Vector";
    }
  }
  else if (type == "Object")
  {
    std::string r;

    LM_M(("KZ: Object"));
    //
    // Either Compound or '{ "type": "xxx", "value": "yyy" }'
    //
    // If the Object contains "value", then it is considered an object, not a compound
    //
    if (iter->value.HasMember("value"))
    {
      LM_M(("KZ: Normal object"));
      r = parseContextAttributeObject(iter->value, caP);
      LM_M(("KZ: Normal object parsed"));
      if (r != "OK")
      {
        LM_W(("Bad Input (JSON parse error in ContextAttribute::Object"));
        ciP->httpStatusCode = SccBadRequest;
        return r;
      }
    }
    else
    {
      LM_M(("KZ: Compound object"));
      parseContextAttributeCompoundValue(iter, caP, NULL);
      caP->valueType = orion::ValueTypeObject;
    }
  }
  else
  {
    LM_W(("Bad Input (bad type for ContextAttribute)"));
    ciP->httpStatusCode = SccBadRequest;
    return "invalid JSON type for ContextAttribute";
  }

  if (caP->name == "")
  {
    LM_W(("Bad Input (no 'name' for ContextAttribute"));
    ciP->httpStatusCode = SccBadRequest;
    return "no 'name' for ContextAttribute";
  }

  return "OK";
}
