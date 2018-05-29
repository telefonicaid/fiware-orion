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

#include "jsonParseV2/parseContextAttributeCompoundValue.h"
#include "jsonParseV2/parseAttributeValue.h"



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
    alarmMgr.badInput(clientIp, "JSON parse error");
    oe.fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    ciP->httpStatusCode = SccBadRequest;
    return oe.toJson();
  }


  if (!document.IsObject() && !document.IsArray())
  {
    alarmMgr.badInput(clientIp, "JSON parse error");
    oe.fill(SccBadRequest, "Neither JSON Object nor JSON Array for attribute::value", "BadRequest");
    ciP->httpStatusCode = SccBadRequest;
    return oe.toJson();
  }

  caP->valueType  = (document.IsObject())? orion::ValueTypeObject : orion::ValueTypeVector;
  parseContextAttributeCompoundValueStandAlone(document, caP, caP->valueType);

  return "OK";
}
