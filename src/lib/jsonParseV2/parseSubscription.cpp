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
* Author: Orion dev team
*/
#include <regex.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "rapidjson/document.h"

#include "alarmMgr/alarmMgr.h"
#include "common/globals.h"
#include "common/errorMessages.h"
#include "common/RenderFormat.h"
#include "common/string.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/Verb.h"
#include "ngsi/Request.h"
#include "parse/forbiddenChars.h"
#include "jsonParseV2/badInput.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/jsonRequestTreat.h"
#include "jsonParseV2/utilsParse.h"
#include "jsonParseV2/parseEntitiesVector.h"
#include "jsonParseV2/parseStringVector.h"
#include "jsonParseV2/parseSubscription.h"
#include "jsonParseV2/parseExpression.h"
#include "jsonParseV2/parseCompoundCommon.h"
#include "jsonParseV2/parseContextAttribute.h"



/* ****************************************************************************
*
* USING
*/
using ngsiv2::SubscriptionUpdate;
using ngsiv2::EntID;
using rapidjson::Value;



/* ****************************************************************************
*
* Prototypes
*/
static std::string parseNotification(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& notification);
static std::string parseSubject(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& subject);
static std::string parseNotifyConditionVector(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& condition);
static std::string parseDictionary(ConnectionInfo*                      ciP,
                                   std::map<std::string, std::string>&  dict,
                                   const Value&                         object,
                                   const std::string&                   name);



/* ****************************************************************************
*
* parseSubscription -
*/
std::string parseSubscription(ConnectionInfo* ciP, SubscriptionUpdate* subsP, bool update)
{
  rapidjson::Document document;

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

  if (document.ObjectEmpty())
  {
    //
    // Initially we used the method "Empty". As the broker crashed inside that method, some
    // research was made and "ObjectEmpty" was found. As the broker stopped crashing and complaints
    // about crashes with small docs and "Empty()" were found on the internet, we opted to use ObjectEmpty
    //
    return badInput(ciP, ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD);
  }


  // Description field

  Opt<std::string> description = getStringOpt(document, "description");
  if (!description.ok())
  {
    return badInput(ciP, description.error);
  }
  else if (description.given)
  {
    std::string descriptionString = description.value;

    if (descriptionString.length() > MAX_DESCRIPTION_LENGTH)
    {
      return badInput(ciP, "max description length exceeded");
    }
    if (forbiddenChars(descriptionString.c_str()))
    {
      return badInput(ciP, "forbidden characters in description");
    }

    subsP->descriptionProvided = true;
    subsP->description = descriptionString;
  }


  // Subject field
  if (document.HasMember("subject"))
  {
    const Value&  subject = document["subject"];
    std::string   r       = parseSubject(ciP, subsP, subject);

    if (!r.empty())
    {
      return r;
    }
  }
  else if (!update)
  {
    return badInput(ciP, "no subject for subscription specified");
  }

  // Notification field
  if (document.HasMember("notification"))
  {
    const Value&  notification = document["notification"];
    std::string   r            = parseNotification(ciP, subsP, notification);

    if (!r.empty())
    {
      return r;
    }
  }
  else if (!update)
  {
    return badInput(ciP, "no notification for subscription specified");
  }

  // Expires field
  Opt<std::string> expiresOpt = getStringOpt(document, "expires");

  if (!expiresOpt.ok())
  {
    return badInput(ciP, expiresOpt.error);
  }
  else if (expiresOpt.given)
  {
    std::string  expires = expiresOpt.value;
    int64_t      eT      = -1;

    if (expires.empty())
    {
      eT = PERMANENT_EXPIRES_DATETIME;
    }
    else
    {
      eT = (int64_t) parse8601Time(expires);
      if (eT == -1)
      {
        return badInput(ciP, "expires has an invalid format");
      }
    }

    subsP->expiresProvided = true;
    subsP->expires = eT;
  }
  else if (!update)
  {
    subsP->expires = PERMANENT_EXPIRES_DATETIME;
  }

  // Status field
  Opt<std::string> statusOpt = getStringOpt(document, "status");
  if (!statusOpt.ok())
  {
    return badInput(ciP, statusOpt.error);
  }
  else if (statusOpt.given)
  {
    std::string statusString = statusOpt.value;

    if ((statusString != "active") && (statusString != "inactive") && (statusString != "oneshot"))
    {
      return badInput(ciP, ERROR_DESC_BAD_REQUEST_INVALID_STATUS);
    }
    subsP->statusProvided = true;
    subsP->status = statusString;
  }

  // Throttling
  Opt<int64_t> throttlingOpt = getInt64Opt(document, "throttling");
  if (!throttlingOpt.ok())
  {
    return badInput(ciP, throttlingOpt.error);
  }
  else if (throttlingOpt.given)
  {
    subsP->throttlingProvided = true;
    subsP->throttling = throttlingOpt.value;
  }
  else if (!update)  // throttling was not set and it is not update
  {
    subsP->throttling = 0;  // Default value if not provided at creation => no throttling
  }

  return "OK";
}



/* ****************************************************************************
*
* parseSubject -
*
*/
static std::string parseSubject(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& subject)
{
  std::string  r;
  bool         b;

  subsP->subjectProvided  = true;

  if (!subject.IsObject())
  {
    return badInput(ciP, "subject is not an object");
  }

  // Entities
  if (!subject.HasMember("entities"))
  {
    return badInput(ciP, "no subject entities specified");
  }

  std::string errorString;
  b = parseEntitiesVector(ciP, &subsP->subject.entities, subject["entities"], &errorString);
  if (b == false)
  {
    return badInput(ciP, errorString);
  }

  if (subsP->subject.entities.size() == 0)
  {
    return badInput(ciP, "subject entities is empty");
  }

  // Condition
  if (subject.HasMember("condition"))
  {
    const Value& condition = subject["condition"];

    if (!condition.IsObject())
    {
      return badInput(ciP, "condition is not an object");
    }

    if (condition.ObjectEmpty())
    {
      return badInput(ciP, "condition is empty");
    }

    r = parseNotifyConditionVector(ciP, subsP, condition);
    if (!r.empty())
    {
      return r;
    }
  }

  return r;
}



/* ****************************************************************************
*
* parseCustomJson -
*
*/
std::string parseCustomJson
(
  const Value&               holder,
  orion::CompoundValueNode**  json
)
{
  // this new memory is freed in in HttpInfo::release()/MqttInfo::release()()
  // or before returning in this function in the case of error
  *json = new orion::CompoundValueNode();

  if (holder.IsArray())
  {
    for (rapidjson::Value::ConstValueIterator iter = holder.Begin(); iter != holder.End(); ++iter)
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

      (*json)->childV.push_back(cvnP);
      (*json)->valueType = orion::ValueTypeVector;

      //
      // Start recursive calls if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        std::string r = parseCompoundValue(iter, cvnP, 0);
        if (r != "OK")
        {
          // Early return
          delete *json;
          return r;
        }
      }
    }
  }
  else if (holder.IsObject())
  {
    for (rapidjson::Value::ConstMemberIterator iter = holder.MemberBegin(); iter != holder.MemberEnd(); ++iter)
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

      (*json)->childV.push_back(cvnP);
      (*json)->valueType = orion::ValueTypeObject;

      //
      // Start recursive calls if Object or Array
      //
      if ((nodeType == "Object") || (nodeType == "Array"))
      {
        std::string r = parseCompoundValue(iter, cvnP, 0);
        if (r != "OK")
        {
          // Early return
          delete *json;
          return r;
        }
      }
    }
  }
  else
  {
    delete *json;
    return "json fields in httpCustom or mqttCustom must be object or array";
  }

  return "";
}



/* ****************************************************************************
*
* parseCustomPayload -
*
* Both for HTTP and MQTT notifications
*/
static std::string parseCustomPayload
(
  ConnectionInfo*             ciP,
  ngsiv2::CustomPayloadType*  payloadType,
  std::string*                payload,
  bool*                       includePayload,
  orion::CompoundValueNode**  json,
  Entity*                     ngsi,
  const Value&                holder
)
{
  // Text is the one by default
  *payloadType = ngsiv2::CustomPayloadType::Text;

  unsigned int n = 0;
  if (holder.HasMember("payload")) n++;
  if (holder.HasMember("json"))    n++;
  if (holder.HasMember("ngsi"))    n++;

  if (n > 1)
  {
    return badInput(ciP, "only one of payload, json or ngsi fields accepted at the same time in httpCustom or mqttCustom");
  }

  if (holder.HasMember("payload"))
  {
    if (isNull(holder, "payload"))
    {
      *includePayload = false;

      // We initialize also payload in this case, although its value is irrelevant
      *payload = "";
    }
    else
    {
      Opt<std::string> payloadOpt = getStringOpt(holder, "payload", "payload custom notification");

      if (!payloadOpt.ok())
      {
        return badInput(ciP, payloadOpt.error);
      }

      if (forbiddenChars(payloadOpt.value.c_str()))
      {
        return badInput(ciP, "forbidden characters in custom /payload/");
      }

      *includePayload = true;
      *payload = payloadOpt.value;
    }
  }
  else  if (holder.HasMember("json"))
  {
    *payloadType = ngsiv2::CustomPayloadType::Json;

    std::string r = parseCustomJson(holder["json"], json);
    if (!r.empty())
    {
      return badInput(ciP, r);
    }
  }
  else if (holder.HasMember("ngsi"))
  {
    *payloadType = ngsiv2::CustomPayloadType::Ngsi;

    for (rapidjson::Value::ConstMemberIterator iter = holder["ngsi"].MemberBegin(); iter != holder["ngsi"].MemberEnd(); ++iter)
    {
      std::string name   = iter->name.GetString();
      std::string type   = jsonParseTypeNames[iter->value.GetType()];

      if (name == "id")
      {
        if (type != "String")
        {
          return badInput(ciP, ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTID);
        }

        ngsi->id = iter->value.GetString();
      }
      else if (name == "type")
      {
        if (type != "String")
        {
          return badInput(ciP, ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTTYPE);
        }

        ngsi->type = iter->value.GetString();
      }
      else  // attribute
      {
        ContextAttribute* caP = new ContextAttribute();

        ngsi->attributeVector.push_back(caP);

        // Note we are using relaxForbiddenCheck true in this case, as JEXL expressions typically use forbidden
        // chars and we don't want to fail in that case
        std::string r = parseContextAttribute(ciP, iter, caP, false, true);

        if (r == "max deep reached")
        {
          return badInput(ciP, ERROR_DESC_PARSE_MAX_JSON_NESTING);
        }
        else if (r != "OK")  // other error cases get a general treatment
        {
          return badInput(ciP, r);
        }

        // only evalPriority metadadata is allowed in this case
        for (unsigned ix = 0; ix < caP->metadataVector.size(); ix++)
        {
          if (caP->metadataVector[ix]->name != NGSI_MD_EVAL_PRIORITY)
          {
            return badInput(ciP, ERROR_DESC_BAD_REQUEST_METADATA_NOT_ALLOWED_CUSTOM_NOTIF);
          }
          else
          {
            if (caP->metadataVector[ix]->valueType != orion::ValueTypeNumber)
            {
              return badInput(ciP, ERROR_DESC_BAD_REQUEST_EVALPRIORITY_MUST_BE_A_NUMBER);
            }
            if (caP->metadataVector[ix]->numberValue < MIN_PRIORITY)
            {
              return badInput(ciP, ERROR_DESC_BAD_REQUEST_EVALPRIORITY_MIN_ERROR);
            }
            if (caP->metadataVector[ix]->numberValue > MAX_PRIORITY)
            {
              return badInput(ciP, ERROR_DESC_BAD_REQUEST_EVALPRIORITY_MAX_ERROR);
            }
          }
        }
      }
    }
  }

  return "";
}



/* ****************************************************************************
*
* parseMqttUrl -
*/
static std::string parseMqttUrl(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& mqtt)
{
  Opt<std::string> urlOpt = getStringMust(mqtt, "url", "url mqtt notification");

  if (!urlOpt.ok())
  {
    return badInput(ciP, urlOpt.error);
  }
  if (!urlOpt.given)
  {
    return badInput(ciP, "mandatory mqtt field /url/");
  }

  if (forbiddenChars(urlOpt.value.c_str()))
  {
    return badInput(ciP, "forbidden characters in mqtt field /url/");
  }

  std::string  host;
  int          port;
  std::string  path;
  std::string  protocol;
  if (!parseUrl(urlOpt.value, host, port, path, protocol))
  {
    return badInput(ciP, "invalid mqtt /url/");
  }
  if (protocol != "mqtt:")
  {
    return badInput(ciP, "http or https URL cannot be used in mqtt notifications");
  }
  if (path != "/")
  {
    return badInput(ciP, "path cannot be used in mqtt url, use topic instead");
  }

  subsP->notification.mqttInfo.url = urlOpt.value;

  return "";
}



/* ****************************************************************************
*
* parseMqttQoS -
*/
static std::string parseMqttQoS(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& mqtt)
{
  Opt<int64_t> qosOpt = getInt64Opt(mqtt, "qos");
  if (!qosOpt.ok())
  {
    return badInput(ciP, qosOpt.error);
  }
  if (qosOpt.given)
  {
    if ((qosOpt.value < 0) || (qosOpt.value > 2))
    {
      return badInput(ciP, "mqtt qos field must be an integer in the 0 to 2 range");
    }
    else
    {
      subsP->notification.mqttInfo.qos = qosOpt.value;
    }
  }
  else
  {
    subsP->notification.mqttInfo.qos = 0;
  }

  return "";
}



/* ****************************************************************************
*
* parseMqttRetain -
*/
static std::string parseMqttRetain(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& mqtt)
{
  Opt<bool> retainOpt = getBoolOpt(mqtt, "retain");
  if (!retainOpt.ok())
  {
    return badInput(ciP, retainOpt.error);
  }
  if (retainOpt.given)
  {
    subsP->notification.mqttInfo.retain = retainOpt.value;
  }
  else
  {
    subsP->notification.mqttInfo.retain = 0;
  }

  return "";
}



/* ****************************************************************************
*
* parseMqttTopic -
*/
static std::string parseMqttTopic(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& mqtt)
{
  Opt<std::string> topicOpt = getStringMust(mqtt, "topic", "topic mqtt notification");

  if (!topicOpt.ok())
  {
    return badInput(ciP, topicOpt.error);
  }
  if (!topicOpt.given)
  {
    return badInput(ciP, "mandatory mqtt field /topic/");
  }

  if (topicOpt.value.empty())
  {
    return badInput(ciP, "empty mqtt field /topic/");
  }

  if (forbiddenMqttTopic(topicOpt.value.c_str()))
  {
    return badInput(ciP, "+ and # are not allowed in mqtt field /topic/");
  }


  if (forbiddenChars(topicOpt.value.c_str()))
  {
    return badInput(ciP, "forbidden characters in mqtt field /topic/");
  }

  subsP->notification.mqttInfo.topic = topicOpt.value;

  return "";
}



/* ****************************************************************************
*
* parseTimeout -
*/
static std::string parseTimeout(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& http)
{
  Opt<int64_t> timeoutOpt = getInt64Opt(http, "timeout");
  if (!timeoutOpt.ok())
  {
    return badInput(ciP, timeoutOpt.error);
  }
  if (timeoutOpt.given)
  {
    if ((timeoutOpt.value < 0) || (timeoutOpt.value > MAX_HTTP_TIMEOUT))
    {
      return badInput(ciP, "timeout field must be an integer between 0 and " + std::to_string(MAX_HTTP_TIMEOUT));
    }
    else
    {
      subsP->notification.httpInfo.timeout = timeoutOpt.value;
    }
  }
  else
  {
    subsP->notification.httpInfo.timeout = 0;
  }

  return "";
}



/* ****************************************************************************
*
* parseMqttAuth -
*/
static std::string parseMqttAuth(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& mqtt)
{
  unsigned int howMany = 0;
  subsP->notification.mqttInfo.providedAuth = false;

  Opt<std::string> userOpt = getStringOpt(mqtt, "user", "user mqtt notification");
  if (!userOpt.ok())
  {
    return badInput(ciP, userOpt.error);
  }

  // Note there is no forbidden chars checking for password. It is not needed: this
  // field is never rendered in the JSON response API, so there is no risk of injection attacks
  if (forbiddenChars(userOpt.value.c_str()))
  {
    return badInput(ciP, "forbidden characters in mqtt /user/");
  }

  if (userOpt.given)
  {
    subsP->notification.mqttInfo.user = userOpt.value;
    howMany++;
  }

  Opt<std::string> passwdOpt = getStringOpt(mqtt, "passwd", "passwd mqtt notification");
  if (!passwdOpt.ok())
  {
    return badInput(ciP, passwdOpt.error);
  }
  if (passwdOpt.given)
  {
    subsP->notification.mqttInfo.passwd = passwdOpt.value;
    howMany++;
  }

  // howMany has to be either 0 (no auth no pass) or 2 (auth and passwd)
  if (howMany == 1)
  {
    return badInput(ciP, "you must use user and passwd fields simultaneously");
  }
  else if (howMany == 2)
  {
    subsP->notification.mqttInfo.providedAuth = true;
  }

  return "";
}



/* ****************************************************************************
*
* parseNotification -
*/
static std::string parseNotification(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& notification)
{
  std::string r;

  subsP->notificationProvided = true;

  if (!notification.IsObject())
  {
    return badInput(ciP, "notification is not an object");
  }

  // Check we have only one type
  int n = 0;
  if (notification.HasMember("http"))        n++;
  if (notification.HasMember("httpCustom"))  n++;
  if (notification.HasMember("mqtt"))        n++;
  if (notification.HasMember("mqttCustom"))  n++;
  if (n > 1)
  {
    return badInput(ciP, "only one of http, httpCustom, mqtt or mqttCustom is allowed");
  }
  else if (n == 0)
  {
    return badInput(ciP, "http, httpCustom, mqtt or mqttCustom is missing");
  }

  // Callback
  if (notification.HasMember("http"))
  {
    const Value& http = notification["http"];

    subsP->notification.type = ngsiv2::HttpNotification;

    if (!http.IsObject())
    {
      return badInput(ciP, "http notification is not an object");
    }

    // http - url
    {
      Opt<std::string> urlOpt = getStringMust(http, "url", "url http notification");

      if (!urlOpt.ok())
      {
        return badInput(ciP, urlOpt.error);
      }

      if (forbiddenChars(urlOpt.value.c_str()))
      {
        return badInput(ciP, "forbidden characters in http field /url/");
      }

      // timeout
      r = parseTimeout(ciP, subsP, http);
      if (!r.empty())
      {
        return r;
      }

      std::string  host;
      int          port;
      std::string  path;
      std::string  protocol;
      if (!parseUrl(urlOpt.value, host, port, path, protocol))
      {
        return badInput(ciP, "Invalid URL parsing notification url");
      }
      if (protocol == "mqtt:")
      {
        return badInput(ciP, "mqtt URL cannot be used in http notifications");
      }

      subsP->notification.httpInfo.url    = urlOpt.value;
      subsP->notification.httpInfo.custom = false;
    }
  }
  else if (notification.HasMember("httpCustom"))
  {
    const Value& httpCustom = notification["httpCustom"];

    subsP->notification.type = ngsiv2::HttpNotification;

    if (!httpCustom.IsObject())
    {
      return badInput(ciP, "httpCustom notification is not an object");
    }

    // URL
    {
      Opt<std::string> urlOpt = getStringMust(httpCustom, "url", "url httpCustom notification");

      if (!urlOpt.ok())
      {
        return badInput(ciP, urlOpt.error);
      }

      if (forbiddenChars(urlOpt.value.c_str()))
      {
        return badInput(ciP, "forbidden characters in custom /url/");
      }

      //
      // Sanity check for custom url:
      // If the url contains custom stuff (any ${xxx}), there is not
      // much we can do.
      // But if not, then the same check as for non-custom url can be used.
      //
      if (strstr(urlOpt.value.c_str(), "${") == NULL)
      {
        std::string  host;
        int          port;
        std::string  path;
        std::string  protocol;
        if (!parseUrl(urlOpt.value, host, port, path, protocol))
        {
          return badInput(ciP, "invalid custom /url/");
        }
        if (protocol == "mqtt:")
        {
          return badInput(ciP, "mqtt URL cannot be used in http notifications");
        }
      }

      subsP->notification.httpInfo.url = urlOpt.value;
    }

    // method -> verb
    {
      Opt<std::string>  methodOpt = getStringOpt(httpCustom, "method", "method httpCustom notification");
      Verb              verb;

      if (!methodOpt.ok())
      {
        return badInput(ciP, methodOpt.error);
      }

      if (methodOpt.given)
      {
        verb = str2Verb(methodOpt.value);

        if (verb == UNKNOWNVERB)
        {
          return badInput(ciP, "unknown method httpCustom notification");
        }
      }
      else  // not given by user, the default one will be used
      {
        verb = NOVERB;
      }

      subsP->notification.httpInfo.verb = verb;
    }

    // payload
    r = parseCustomPayload(ciP,
                           &subsP->notification.httpInfo.payloadType,
                           &subsP->notification.httpInfo.payload,
                           &subsP->notification.httpInfo.includePayload,
                           &subsP->notification.httpInfo.json,
                           &subsP->notification.httpInfo.ngsi,
                           httpCustom);
    if (!r.empty())
    {
      return r;
    }

    // qs
    if (httpCustom.HasMember("qs"))
    {
      const Value& qs = httpCustom["qs"];

      if (!qs.IsObject())
      {
        return badInput(ciP, "notification httpCustom qs is not an object");
      }

      if (qs.ObjectEmpty())
      {
        return badInput(ciP, "notification httpCustom qs is empty");
      }

      std::string r = parseDictionary(ciP, subsP->notification.httpInfo.qs, qs, "notification httpCustom qs");

      if (!r.empty())
      {
        return r;
      }
    }

    // headers
    if (httpCustom.HasMember("headers"))
    {
      const Value& headers = httpCustom["headers"];

      if (!headers.IsObject())
      {
        return badInput(ciP, "notification httpCustom headers is not an object");
      }

      if (headers.ObjectEmpty())
      {
        return badInput(ciP, "notification httpCustom headers is empty");
      }

      std::string r = parseDictionary(ciP,
                                      subsP->notification.httpInfo.headers,
                                      headers,
                                      "notification httpCustom headers");

      if (!r.empty())
      {
        return r;
      }
    }

    // timeout
    r = parseTimeout(ciP, subsP, httpCustom);
    if (!r.empty())
    {
      return r;
    }

    subsP->notification.httpInfo.custom = true;
  }
  else if (notification.HasMember("mqtt"))
  {
    subsP->notification.type = ngsiv2::MqttNotification;

    const Value& mqtt = notification["mqtt"];

    if (!mqtt.IsObject())
    {
      return badInput(ciP, "mqtt notification is not an object");
    }

    // url
    r = parseMqttUrl(ciP, subsP, mqtt);
    if (!r.empty())
    {
      return r;
    }

    // user/pass
    r = parseMqttAuth(ciP, subsP, mqtt);
    if (!r.empty())
    {
      return r;
    }

    // qos
    r = parseMqttQoS(ciP, subsP, mqtt);
    if (!r.empty())
    {
      return r;
    }

    // retain
    r = parseMqttRetain(ciP, subsP, mqtt);
    if (!r.empty())
    {
      return r;
    }

    // topic
    r = parseMqttTopic(ciP, subsP, mqtt);
    if (!r.empty())
    {
      return r;
    }

    subsP->notification.mqttInfo.custom = false;
  }
  else if (notification.HasMember("mqttCustom"))
  {
    subsP->notification.type = ngsiv2::MqttNotification;

    const Value& mqttCustom = notification["mqttCustom"];

    if (!mqttCustom.IsObject())
    {
      return badInput(ciP, "mqttCustom notification is not an object");
    }

    // url (same as in not custom mqtt)
    r = parseMqttUrl(ciP, subsP, mqttCustom);
    if (!r.empty())
    {
      return r;
    }

    // user/pass same as in not custom mqtt)
    r = parseMqttAuth(ciP, subsP, mqttCustom);
    if (!r.empty())
    {
      return r;
    }

    // qos (same as in not custom mqtt)
    r = parseMqttQoS(ciP, subsP, mqttCustom);
    if (!r.empty())
    {
      return r;
    }

    // retain
    r = parseMqttRetain(ciP, subsP, mqttCustom);
    if (!r.empty())
    {
      return r;
    }

    // topic (same as in not custom mqtt)
    r = parseMqttTopic(ciP, subsP, mqttCustom);
    if (!r.empty())
    {
      return r;
    }

    // payload
    r = parseCustomPayload(ciP,
                           &subsP->notification.mqttInfo.payloadType,
                           &subsP->notification.mqttInfo.payload,
                           &subsP->notification.mqttInfo.includePayload,
                           &subsP->notification.mqttInfo.json,
                           &subsP->notification.mqttInfo.ngsi,
                           mqttCustom);

    if (!r.empty())
    {
      return r;
    }

    subsP->notification.mqttInfo.custom = true;
  }

  // Attributes
  std::string errorString;

  if (notification.HasMember("attrs") && notification.HasMember("exceptAttrs"))
  {
    return badInput(ciP, "http notification has attrs and exceptAttrs");
  }

  if (notification.HasMember("attrs"))
  {
    bool b = parseStringVector(&subsP->notification.attributes,
                               notification["attrs"],
                               "attrs",
                               true,
                               true,
                               &errorString);

    if (b == false)
    {
      return badInput(ciP, errorString);
    }
    subsP->notification.blacklist = false;
    subsP->blacklistProvided      = true;
  }
  else if (notification.HasMember("exceptAttrs"))
  {
    if (parseStringVector(&subsP->notification.attributes,
                          notification["exceptAttrs"],
                          "exceptAttrs",
                          true,
                          true,
                          &errorString) == false)
    {
      return badInput(ciP, errorString);
    }

    if (subsP->notification.attributes.empty())
    {
      return badInput(ciP, "http notification has exceptAttrs is empty");
    }
    subsP->notification.blacklist = true;
    subsP->blacklistProvided      = true;
  }
  if (notification.HasMember("onlyChangedAttrs"))
  {
    Opt<bool> onlyChangedOpt = getBoolOpt(notification, "onlyChangedAttrs");
    if (!onlyChangedOpt.ok())
    {
      return badInput(ciP, onlyChangedOpt.error);
    }
    else if (onlyChangedOpt.given)
    {
      bool onlyChangedBool = onlyChangedOpt.value;
      subsP->onlyChangedProvided = true;
      subsP->notification.onlyChanged = onlyChangedBool;
    }
  }

  // covered
  if (notification.HasMember("covered"))
  {
    Opt<bool> coveredOpt = getBoolOpt(notification, "covered");
    if (!coveredOpt.ok())
    {
      return badInput(ciP, coveredOpt.error);
    }
    else if (coveredOpt.given)
    {
      bool coveredBool = coveredOpt.value;
      subsP->coveredProvided = true;
      subsP->notification.covered = coveredBool;

      if ((subsP->notification.covered) && (subsP->notification.attributes.size() == 0))
      {
        return badInput(ciP, "covered true cannot be used if notification attributes list is empty");
      }
    }
  }

  // metadata
  if (notification.HasMember("metadata"))
  {
    bool b = parseStringVector(&subsP->notification.metadata,
                               notification["metadata"],
                               "metadata",
                               true,
                               true,
                               &errorString);

    if (b == false)
    {
      return badInput(ciP, errorString);
    }
  }

  // attrsFormat field
  Opt<std::string>  attrsFormatOpt = getStringOpt(notification, "attrsFormat");

  if (!attrsFormatOpt.ok())
  {
    return badInput(ciP, attrsFormatOpt.error);
  }
  else if (attrsFormatOpt.given)
  {
    std::string   attrsFormatString = attrsFormatOpt.value;
    RenderFormat  nFormat           = stringToRenderFormat(attrsFormatString, true);

    if (nFormat == NO_FORMAT)
    {
      return badInput(ciP, ERROR_DESC_BAD_REQUEST_INVALID_ATTRSFORMAT);
    }
    subsP->attrsFormatProvided = true;
    subsP->attrsFormat = nFormat;
  }
  else  // Default value for creation
  {
    subsP->attrsFormatProvided = true;
    subsP->attrsFormat = DEFAULT_RENDER_FORMAT;  // Default format for NGSIv2: normalized
  }

  // MaxFailsLimit
  Opt<int64_t> maxFailsLimitOpt = getInt64Opt(notification, "maxFailsLimit");
  if (!maxFailsLimitOpt.ok())
  {
    return badInput(ciP, maxFailsLimitOpt.error);
  }
  else if (maxFailsLimitOpt.given)
  {
    if (maxFailsLimitOpt.value <= 0)
    {
      return badInput(ciP, "maxFailsLimit must be greater than zero");
    }

    subsP->notification.maxFailsLimit = maxFailsLimitOpt.value;
  }

  return "";
}



/* ****************************************************************************
*
* parseNotifyConditionVector -
*/
static std::string parseNotifyConditionVector
(
  ConnectionInfo*              ciP,
  ngsiv2::SubscriptionUpdate*  subsP,
  const Value&                 condition
)
{
  if (!condition.IsObject())
  {
    return badInput(ciP, "condition is not an object");
  }

  // Attributes
  if (condition.HasMember("attrs"))
  {
    std::string errorString;
    bool        b = parseStringVector(&subsP->subject.condition.attributes,
                                      condition["attrs"],
                                      "attrs",
                                      true,
                                      true,
                                      &errorString);
    if (b == false)
    {
      return badInput(ciP, errorString);
    }
  }

  // Expression
  if (condition.HasMember("expression"))
  {
    std::string r = parseExpression(condition["expression"], &subsP->restriction.scopeVector, subsP);

    if (r != "OK")
    {
      return badInput(ciP, r);
    }
  }

  // Operations
  if (condition.HasMember("alterationTypes"))
  {
    std::string errorString;
    std::vector<std::string> altTypeStrings;
    bool        b = parseStringVector(&altTypeStrings,
                                      condition["alterationTypes"],
                                      "alterationTypes",
                                      true,
                                      true,
                                      &errorString);
    if (b == false)
    {
      return badInput(ciP, errorString);
    }

    for (unsigned int ix = 0; ix < altTypeStrings.size(); ix++)
    {
      ngsiv2::SubAltType altType = parseAlterationType(altTypeStrings[ix]);
      if (altType == ngsiv2::SubAltType::Unknown)
      {
        return badInput(ciP, "unknown subscription alterationType: " + altTypeStrings[ix]);
      }
      else
      {
        subsP->subject.condition.altTypes.push_back(altType);
      }
    }
  }

  // notifyOnMetadataChange
  if (condition.HasMember("notifyOnMetadataChange"))
  {
    Opt<bool> notifyOnMetadataChangeOpt = getBoolOpt(condition, "notifyOnMetadataChange");
    if (!notifyOnMetadataChangeOpt.ok())
    {
      return badInput(ciP, notifyOnMetadataChangeOpt.error);
    }
    else if (notifyOnMetadataChangeOpt.given)
    {
      bool notifyOnMetadataChangeBool = notifyOnMetadataChangeOpt.value;
      subsP->notifyOnMetadataChangeProvided = true;
      subsP->subject.condition.notifyOnMetadataChange = notifyOnMetadataChangeBool;
    }
  }

  return "";
}



/* ****************************************************************************
*
* parseDictionary -
*/
static std::string parseDictionary
(
  ConnectionInfo*                      ciP,
  std::map<std::string, std::string>&  dict,
  const Value&                         object,
  const std::string&                   name
)
{
  if (!object.IsObject())
  {
    return badInput(ciP, name + " is not an object");
  }

  for (Value::ConstMemberIterator iter = object.MemberBegin(); iter != object.MemberEnd(); ++iter)
  {
    if (!iter->value.IsString())
    {
      return badInput(ciP, name + " element is not a string");
    }

    std::string value = iter->value.GetString();
    std::string key   = iter->name.GetString();

    if (forbiddenChars(value.c_str()))
    {
      return badInput(ciP, std::string("forbidden characters in custom ") + name);
    }

    if (forbiddenChars(key.c_str()))
    {
      return badInput(ciP, std::string("forbidden characters in custom ") + name);
    }

    dict[iter->name.GetString()] = value;
  }

  return "";
}
