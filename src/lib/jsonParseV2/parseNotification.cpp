/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "apiTypesV2/Entity.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi10/NotifyContextRequest.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/HttpStatusCode.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseEntityObject.h"
#include "jsonParseV2/parseNotification.h"



/* ****************************************************************************
*
* parseContextElementResponse -
*/
static bool parseContextElementResponse
(
  ConnectionInfo*                      ciP,
  rapidjson::Value::ConstValueIterator iter,
  ContextElementResponse*              cerP,
  OrionError*                          oeP
)
{
  std::string type = jsonParseTypeNames[iter->GetType()];

  if (type != "Object")
  {
    oeP->fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_DATA_ITEM_NOT_OBJECT);
    return false;
  }

  //
  // NOTE
  //   Let's use the function parseEntityObject(), that is already implemented for a similar case.
  //   Only problem is that 'parseEntityObject' takes an Entity* as input while ContextElement contains an EntityId
  //   This is because we're using an old V1 type for notifications (NotifyContextRequest).
  //   If we instead create a *new* type for V2 notifications we could use parseEntityObject(), without any problem.
  //
  //   However, we can still use parseEntityObject(), it's just that we need to convert
  //   the Entity into an EntityId afterwards.
  //
  std::string     r;
  Entity          entity;

  if ((r = parseEntityObject(ciP, iter, &entity, false, true)) != "OK")
  {
    oeP->fill(SccBadRequest, r);
    return false;
  }

  cerP->entity.fill(entity.id, entity.type, entity.isPattern);
  cerP->entity.attributeVector.push_back(entity.attributeVector);
  entity.release();

  return true;
}



/* ****************************************************************************
*
* parseNotificationData -
*/
static bool parseNotificationData
(
  ConnectionInfo*                       ciP,
  rapidjson::Value::ConstMemberIterator iter,
  NotifyContextRequest*                 ncrP,
  OrionError*                           oeP
)
{
  std::string type = jsonParseTypeNames[iter->value.GetType()];

  if (type != "Array")
  {
    oeP->fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_DATA_NOT_ARRAY);
    return false;
  }

  for (rapidjson::Value::ConstValueIterator iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2)
  {
    ContextElementResponse* cerP = new ContextElementResponse();

    ncrP->contextElementResponseVector.vec.push_back(cerP);

    if (parseContextElementResponse(ciP, iter2, cerP, oeP) == false)
    {
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* parseNotificationNormalized -
*/
static bool parseNotificationNormalized(ConnectionInfo* ciP, NotifyContextRequest* ncrP, OrionError* oeP)
{
  rapidjson::Document  document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error");
    oeP->fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }

  if (!document.IsObject())
  {
    alarmMgr.badInput(clientIp, "JSON Parse Error");
    oeP->fill(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }
  else if (document.ObjectEmpty())
  {
    alarmMgr.badInput(clientIp, "Empty JSON payload");
    oeP->fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }
  else if (!document.HasMember("subscriptionId"))
  {
    alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_NO_SUBSCRIPTION_ID);
    oeP->fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_NO_SUBSCRIPTION_ID, "BadRequest");
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }
  else if (!document.HasMember("data"))
  {
    std::string  details = "Invalid JSON payload, mandatory field /data/ not found";

    alarmMgr.badInput(clientIp, details);
    oeP->fill(SccBadRequest, details, "BadRequest");
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }

  for (rapidjson::Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "subscriptionId")
    {
      if (type != "String")
      {
        oeP->fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_SUBSCRIPTIONID_NOT_STRING, "BadRequest");

        alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_SUBSCRIPTIONID_NOT_STRING);
        ciP->httpStatusCode = SccBadRequest;

        return false;
      }

      ncrP->subscriptionId.set(iter->value.GetString());
    }
    else if (name == "data")
    {
      if (type != "Array")
      {
        oeP->fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_DATA_NOT_ARRAY, "BadRequest");

        alarmMgr.badInput(clientIp, ERROR_DESC_BAD_REQUEST_DATA_NOT_ARRAY);
        ciP->httpStatusCode = SccBadRequest;

        return false;
      }

      if (parseNotificationData(ciP, iter, ncrP, oeP) == false)
      {
        alarmMgr.badInput(clientIp, oeP->details);
        ciP->httpStatusCode = SccBadRequest;

        return false;
      }
    }
    else
    {
      std::string  description = std::string("Unrecognized field in JSON payload: /") + name + "/";

      alarmMgr.badInput(clientIp, description);
      oeP->fill(SccBadRequest, description, "BadRequest");
      ciP->httpStatusCode = SccBadRequest;

      return false;
    }
  }

  return true;
}




/* ****************************************************************************
*
* parseNotification -
*/
std::string parseNotification(ConnectionInfo* ciP, NotifyContextRequest* ncrP)
{
  OrionError  oe;
  bool        ok = false;

  if ((ciP->httpHeaders.ngsiv2AttrsFormat == "normalized") || (ciP->httpHeaders.ngsiv2AttrsFormat == ""))
  {
    ok = parseNotificationNormalized(ciP, ncrP, &oe);
  }
  else if (ciP->httpHeaders.ngsiv2AttrsFormat == "keyValues")
  {
    oe.fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_FORMAT_KEYVALUES);
  }
  else if (ciP->httpHeaders.ngsiv2AttrsFormat == "uniqueValues")
  {
    oe.fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_FORMAT_UNIQUEVALUES);
  }
  else if (ciP->httpHeaders.ngsiv2AttrsFormat == "custom")
  {
    oe.fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_FORMAT_CUSTOM);
  }
  else
  {
    oe.fill(SccBadRequest, ERROR_DESC_BAD_REQUEST_FORMAT_INVALID);
  }

  if (ok == true)
  {
    return "OK";  // FIXME #3151: parseNotification should return bool and have an input param "OrionError* oeP"
  }

  return oe.toJson();
}
