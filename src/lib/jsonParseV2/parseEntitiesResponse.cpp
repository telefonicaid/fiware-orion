/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "apiTypesV2/Entities.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "jsonParseV2/parseEntityObject.h"
#include "jsonParseV2/parseEntityVector.h"
#include "jsonParseV2/parseEntitiesResponse.h"



/* ****************************************************************************
*
* parseEntitiesResponse -
*
* This function is used to parse the response of a forwarded query 
* - GET /v2/entities
*
*/
bool parseEntitiesResponse(ConnectionInfo* ciP, const char* payload, Entities* evP, OrionError* oeP)
{
  rapidjson::Document  document;

  LM_T(LmtForward, ("parsing forwarded response: %s", payload));

  document.Parse(payload);

  LM_T(LmtForward, ("parsed forwarded response"));

  if (document.HasParseError())
  {
    oeP->fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    alarmMgr.badInput(clientIp, "JSON parse error");
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }

  if (!document.IsArray())
  {
    oeP->fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);

    alarmMgr.badInput(clientIp, "JSON Parse Error");
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }

#if 1
  for (rapidjson::Value::ConstValueIterator iter = document.Begin(); iter != document.End(); ++iter)
  {
    Entity* eP = new Entity();

    std::string s = parseEntityObject(ciP, iter, eP, true);
    if (s != "OK")
    {
      oeP->fill(SccBadRequest, s);
      alarmMgr.badInput(clientIp, "JSON Parse Error");
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
    
    evP->vec.push_back(eP);
  }
#else
  std::string  errorString;
  LM_T(LmtForward, ("Getting iter"));
  rapidjson::Value::ConstMemberIterator iter = document.MemberBegin();
  LM_T(LmtForward, ("calling parseEntityVector with iter"));
  std::string  s = parseEntityVector(ciP, iter, evP, true);
  LM_T(LmtForward, ("After parseEntityVector"));

  if (s != "OK")
  {
    oeP->fill(SccBadRequest, s);
    alarmMgr.badInput(clientIp, "JSON Parse Error");
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }
#endif

  return true;
}
