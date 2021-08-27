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

    alarmMgr.badInput(clientIp, "JSON parse error");
    ciP->httpStatusCode = SccBadRequest;

    return oe.toJson();
  }

  if (!document.IsObject())
  {
    OrionError oe(SccBadRequest, ERROR_DESC_PARSE, ERROR_PARSE);

    alarmMgr.badInput(clientIp, "JSON parse error");
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

  // MaxFailsLimit
  Opt<int64_t> maxFailsLimitOpt = getInt64Opt(document, "maxFailsLimit");
  if (!maxFailsLimitOpt.ok())
  {
    return badInput(ciP, maxFailsLimitOpt.error);
  }
  else if (maxFailsLimitOpt.given)
  {
    subsP->maxFailsLimitProvided = true;
    subsP->maxFailsLimit = maxFailsLimitOpt.value;
  }
  else if (!update)  // maxFailsLimit was not set and it is not update
  {
    subsP->maxFailsLimit = 0;  // Default value if not provided at creation => no maxFailsLimit
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
* parseNotification -
*/
static std::string parseNotification(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& notification)
{
  subsP->notificationProvided = true;

  if (!notification.IsObject())
  {
    return badInput(ciP, "notification is not an object");
  }

  // Callback
  if (notification.HasMember("http") && notification.HasMember("httpCustom"))
  {
    return badInput(ciP, "notification has http and httpCustom");
  }
  else if (notification.HasMember("http"))
  {
    const Value& http = notification["http"];

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

      if (!validUrl(urlOpt.value))
      {
        return badInput(ciP, "Invalid URL parsing notification url");
      }

      subsP->notification.httpInfo.url    = urlOpt.value;
      subsP->notification.httpInfo.custom = false;
    }
  }
  else if (notification.HasMember("httpCustom"))
  {
    const Value& httpCustom = notification["httpCustom"];

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
        if (!validUrl(urlOpt.value))
        {
          return badInput(ciP, "invalid custom /url/");
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
    {
      if (isNull(httpCustom, "payload"))
      {
        subsP->notification.httpInfo.includePayload = false;

        // We initialize also httpInfo.payload in this case, although its value is irrelevant
        subsP->notification.httpInfo.payload = "";
      }
      else
      {
        Opt<std::string> payloadOpt = getStringOpt(httpCustom, "payload", "payload httpCustom notification");

        if (!payloadOpt.ok())
        {
          return badInput(ciP, payloadOpt.error);
        }

        if (forbiddenChars(payloadOpt.value.c_str()))
        {
          return badInput(ciP, "forbidden characters in custom /payload/");
        }

        subsP->notification.httpInfo.includePayload = true;
        subsP->notification.httpInfo.payload = payloadOpt.value;
      }
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

    subsP->notification.httpInfo.custom = true;
  }
  else  // missing callback field
  {
    return badInput(ciP, "http notification is missing");
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

  if (condition.HasMember("expression"))
  {
    std::string r = parseExpression(condition["expression"], &subsP->restriction.scopeVector, subsP);

    if (r != "OK")
    {
      return badInput(ciP, r);
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
