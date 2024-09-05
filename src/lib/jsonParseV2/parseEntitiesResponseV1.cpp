/*
*
* Copyright 2024 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>

#include "rapidjson/document.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/errorMessages.h"
#include "alarmMgr/AlarmManager.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseEntitiesResponseV1.h"
#include "jsonParseV2/utilsParse.h"
#include "jsonParseV2/parseCompoundCommon.h"



/* ****************************************************************************
*
* prepareContextAttributeCompoundRoot -
*
* Copied from parseContextAttribute.cpp. Given this file is about deprecated functionality and eventually
* will be removed, no need of moving this to a common lib.
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
* parseMetadata -
*
*/
static std::string parseMetadata(ConnectionInfo* ciP, rapidjson::Value::ConstValueIterator&  valueP, Metadata* mP)
{
  for (rapidjson::Value::ConstMemberIterator iter = valueP->MemberBegin(); iter != valueP->MemberEnd(); ++iter)
  {
    std::string name = iter->name.GetString();
    std::string type = jsonParseTypeNames[iter->value.GetType()];

    // keys different from name, type and value are ignored
    if (name == "name")
    {
      if (type != "String")
      {
        return "metadata name  must be a string";
      }
      mP->name = iter->value.GetString();
    }
    if (name == "type")
    {
      if (type != "String")
      {
        return "metadata type must be a string";
      }
      mP->type = iter->value.GetString();
    }
    if (name == "value")
    {
      if (type == "String")
      {
        mP->stringValue  = iter->value.GetString();
        mP->valueType    = orion::ValueTypeString;
      }
      else if (type == "Number")
      {
        mP->numberValue  = iter->value.GetDouble();
        mP->valueType    = orion::ValueTypeNumber;
      }
      else if (type == "True")
      {
        mP->boolValue    = true;
        mP->valueType    = orion::ValueTypeBoolean;
      }
      else if (type == "False")
      {
        mP->boolValue    = false;
        mP->valueType    = orion::ValueTypeBoolean;
      }
      else if (type == "Null")
      {
        mP->valueType    = orion::ValueTypeNull;
      }
      else if ((type == "Array") || (type == "Object"))
      {
        mP->valueType = orion::ValueTypeObject;  // Used both for Array and Object ...

        std::string type   = jsonParseTypeNames[iter->value.GetType()];

        mP->compoundValueP            = new orion::CompoundValueNode();
        mP->compoundValueP->name      = "";
        mP->compoundValueP->valueType = stringToCompoundType(type);

        std::string r  = parseCompoundValue(iter, mP->compoundValueP, 0);

        if (r != "OK")
        {
          return r;
        }
      }
    }
  }

  if ((mP->name.empty()) || (mP->type.empty()))
  {
    return "metadata must have name and type";
  }

  return "OK";
}



/* ****************************************************************************
*
* parseAttribute -
*
*/
static std::string parseAttribute(ConnectionInfo* ciP, rapidjson::Value::ConstValueIterator&  valueP, ContextAttribute* caP)
{
  for (rapidjson::Value::ConstMemberIterator iter = valueP->MemberBegin(); iter != valueP->MemberEnd(); ++iter)
  {
    std::string name = iter->name.GetString();
    std::string type = jsonParseTypeNames[iter->value.GetType()];

    // keys different from metadatas, name, type and value are ignored
    if (name == "metadatas")
    {
      if (type != "Array")
      {
        return "metadatas must be an array";
      }

      for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
      {
        if (!iter2->IsObject())
        {
          return "metadatas item must be an object";
        }

        Metadata *mP = new Metadata();

        std::string s = parseMetadata(ciP, iter2, mP);
        if (s != "OK")
        {
          mP->release();
          delete mP;
          return s;
        }

        caP->metadataVector.push_back(mP);
      }
    }
    if (name == "name")
    {
      if (type != "String")
      {
        return "attribute name  must be a string";
      }
      caP->name = iter->value.GetString();
    }
    if (name == "type")
    {
      if (type != "String")
      {
        return "attribute type must be a string";
      }
      caP->type = iter->value.GetString();
    }
    if (name == "value")
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
  }

  if ((caP->name.empty()) || (caP->type.empty()))
  {
    return "attribute must have name and type";
  }

  return "OK";
}



/* ****************************************************************************
*
* parseEntity -
*
*/
static std::string parseEntity(ConnectionInfo* ciP, rapidjson::Value::ConstMemberIterator&  valueP, Entity* eP)
{
  for (rapidjson::Value::ConstMemberIterator iter = valueP->value.MemberBegin(); iter != valueP->value.MemberEnd(); ++iter)
  {
    std::string name = iter->name.GetString();
    std::string type = jsonParseTypeNames[iter->value.GetType()];

    // keys different from attributes, id and types are ignored (e.g. isPattern)
    if (name == "attributes")
    {
      if (type != "Array")
      {
        return "attributes must be an array";
      }

      for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
      {
        if (!iter2->IsObject())
        {
          return "attributes item must be an object";
        }

        ContextAttribute *caP = new ContextAttribute();

        std::string s = parseAttribute(ciP, iter2, caP);
        if (s != "OK")
        {
          caP->release();
          delete caP;
          return s;
        }

        eP->attributeVector.push_back(caP);
      }
    }
    if (name == "id")
    {
      if (type != "String")
      {
        return "entity id must be a string";
      }
      eP->entityId.id = iter->value.GetString();
    }
    if (name == "type")
    {
      if (type != "String")
      {
        return "entity type must be a string";
      }
      eP->entityId.type = iter->value.GetString();
    }
  }

  if ((eP->entityId.id.empty()) || (eP->entityId.type.empty()))
  {
    return "entity must have id and type";
  }

  return "OK";
}



/* ****************************************************************************
*
* parseEntitiesResponseV1 -
*
* This function is to parse responses for POST /v1/queryContext when that request is used
* in legacyForwarding: true scenarios. This function is called only from postQueryContext() logic
* and will be removed if some day the legacyForwarding: true functionality is fully removed
*
* Example of payload response (for reference)
*
* {
*     "contextResponses": [
*         {
*             "contextElement": {
*                 "attributes": [
*                     {
*                         "name": "lightstatus",
*                         "type": "light",
*                         "value": "L23"
*                     },
*                     {
*                         "name": "pressure",
*                         "type": "clima",
*                         "value": "p23"
*                     },
*                     {
*                         "name": "temperature",
*                         "type": "degree",
*                         "value": "14",
*                         "metadatas": [
*                             {
*                                 "name": "ID",
*                                 "type": "Text",
*                                 "value": "ID2"
*                             }
*                         ],
*                     }
*                 ],
*                 "id": "ConferenceRoom",
*                 "isPattern": "false",
*                 "type": "Room"
*             },
*             "statusCode": {
*                 "code": "200",
*                 "reasonPhrase": "OK"
*             }
*         },
*         {
*             "contextElement": {
*                 "attributes": [
*                     {
*                         "name": "temperature",
*                         "type": "degree",
*                         "value": "14"
*                     }
*                 ],
*                 "id": "ConferenceRoom2",
*                 "isPattern": "false",
*                 "type": "Room"
*             },
*             "statusCode": {
*                 "code": "200",
*                 "reasonPhrase": "OK"
*             }
*         }
*     ]
* }
*
*/
bool parseEntitiesResponseV1(ConnectionInfo* ciP, const char* payload, EntityVector* evP, OrionError* oeP)
{
  rapidjson::Document  document;

  document.Parse(payload);

  if (document.HasParseError())
  {
    oeP->fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    alarmMgr.badInput(clientIp, "JSON Parse Error", parseErrorString(document.GetParseError()));
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (!document.IsObject())
  {
    oeP->fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    alarmMgr.badInput(clientIp, "JSON Parse Error", "JSON Object not found");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name  = iter->name.GetString();
    std::string type  = jsonParseTypeNames[iter->value.GetType()];

    if ((name != "contextResponses") || (type != "Array"))
    {
      oeP->fill(SccBadRequest, "contextResponses key not found or its value is not an array");
      alarmMgr.badInput(clientIp, "JSON Parse Error", "contextResponses key not found or its value is not an array");
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }

    for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
    {
      if (!iter2->IsObject())
      {
        oeP->fill(SccBadRequest, "contextResponses item is not an object");
        alarmMgr.badInput(clientIp, "JSON Parse Error", "contextResponses item is not an object");
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }

      for (rapidjson::Value::ConstMemberIterator iter3 = iter2->MemberBegin(); iter3 != iter2->MemberEnd(); ++iter3)
      {
        std::string name = iter3->name.GetString();
        std::string type = jsonParseTypeNames[iter3->value.GetType()];

        // keys different from contextElement and statusCode are ignored
        if (name == "contextElement")
        {
          if (type != "Object")
          {
            oeP->fill(SccBadRequest, "contextElement value is not an object");
            alarmMgr.badInput(clientIp, "JSON Parse Error", "contextResponses item is not an object");
            ciP->httpStatusCode = SccBadRequest;
            return false;
          }

          Entity* eP = new Entity();
          std::string s = parseEntity(ciP, iter3, eP);

          if (s != "OK")
          {
            eP->release();
            delete eP;
            oeP->fill(SccBadRequest, s);
            alarmMgr.badInput(clientIp, "JSON Parse Error", s);
            ciP->httpStatusCode = SccBadRequest;
            return false;
          }

          evP->vec.push_back(eP);
        }

        if (name == "statusCode")
        {
          if (type != "Object")
          {
            oeP->fill(SccBadRequest, "statusCode value is not an object");
            alarmMgr.badInput(clientIp, "JSON Parse Error", "statusCode value is not an object");
            ciP->httpStatusCode = SccBadRequest;
            return false;
          }

          for (rapidjson::Value::ConstMemberIterator iter4 = iter3->value.MemberBegin(); iter4 != iter3->value.MemberEnd(); ++iter4)
          {
            std::string name = iter4->name.GetString();
            std::string type = jsonParseTypeNames[iter4->value.GetType()];

            // keys different from code is ignored
            if (name == "code")
            {
              if (type != "String")
              {
                oeP->fill(SccBadRequest, "code value is not an string");
                alarmMgr.badInput(clientIp, "JSON Parse Error", "code value is not an string");
                ciP->httpStatusCode = SccBadRequest;
                return false;
              }
              
              std::string code = iter4->value.GetString();
              if (code != "200")
              {
                oeP->fill(SccBadRequest, "non-200 code: " + code);
                alarmMgr.badInput(clientIp, "JSON Parse Error", "non-200 code: " + code);
                ciP->httpStatusCode = SccBadRequest;
                return false;
              }
            }
          }
        }
      }
    }
  }

  return true;
}
