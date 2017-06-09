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
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/jsonRequestTreat.h"
#include "jsonParseV2/utilsParse.h"
#include "jsonParseV2/parseSubscription.h"



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
static std::string parseAttributeList(ConnectionInfo* ciP, std::vector<std::string>* vec, const Value& attributes);
static std::string parseNotification(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& notification);
static std::string parseSubject(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& subject);
static std::string parseEntitiesVector(ConnectionInfo* ciP, std::vector<EntID>* eivP, const Value& entities);
static std::string parseNotifyConditionVector(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& condition);
static std::string badInput(ConnectionInfo* ciP, const std::string& msg);
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

    if (r != "")
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

    if (r != "")
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
        eT = PERMANENT_SUBS_DATETIME;
    }
    else
    {
      eT = parse8601Time(expires);
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
    subsP->expires = PERMANENT_SUBS_DATETIME;
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

    if ((statusString != "active") && (statusString != "inactive"))
    {
      return badInput(ciP, "status is not valid (it has to be either active or inactive)");
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
  std::string r;

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

  r = parseEntitiesVector(ciP, &subsP->subject.entities, subject["entities"]);
  if (r != "")
  {
    return r;
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
    if (r != "")
    {
      return r;
    }
  }

  return r;
}



/* ****************************************************************************
*
* parseEntitiesVector -
*
*/
static std::string parseEntitiesVector(ConnectionInfo* ciP, std::vector<EntID>* eivP, const Value& entities)
{
  if (!entities.IsArray())
  {
    return badInput(ciP, "subject entities is not an array");
  }

  for (Value::ConstValueIterator iter = entities.Begin(); iter != entities.End(); ++iter)
  {
    if (!iter->IsObject())
    {
      return badInput(ciP, "subject entities element is not an object");
    }

    if (!iter->HasMember("id") && !iter->HasMember("idPattern"))
    {
      return badInput(ciP, "subject entities element does not have id nor idPattern");
    }

    if (iter->HasMember("id") && iter->HasMember("idPattern"))
    {
      return badInput(ciP, "subject entities element has id and idPattern");
    }

    if (iter->HasMember("type") && iter->HasMember("typePattern"))
    {
      return badInput(ciP, "subject entities element has type and typePattern");
    }


    std::string  id;
    std::string  idPattern;
    std::string  type;
    std::string  typePattern;

    {
      Opt<std::string> idOpt = getStringOpt(*iter, "id", "subject entities element id");

      if (!idOpt.ok())
      {
        return badInput(ciP, idOpt.error);
      }
      else if (idOpt.given)
      {
        if (idOpt.value.empty())
        {
          return badInput(ciP, "subject entities element id is empty");
        }
        if (forbiddenIdCharsV2(idOpt.value.c_str()))
        {
          return badInput(ciP, "forbidden characters in subject entities element id");
        }
        if (idOpt.value.length() > MAX_ID_LEN)
        {
          return badInput(ciP, "max id length exceeded");
        }
        id = idOpt.value;
      }
    }

    {
      Opt<std::string> idPatOpt = getStringOpt(*iter, "idPattern", "subject entities element idPattern");

      if (!idPatOpt.ok())
      {
        return badInput(ciP, idPatOpt.error);
      }
      else if (idPatOpt.given)
      {
        if (idPatOpt.value.empty())
        {
          return badInput(ciP, "subject entities element idPattern is empty");
        }

        idPattern = idPatOpt.value;

        // FIXME P5: Keep the regex and propagate to sub-cache
        regex_t re;
        if (regcomp(&re, idPattern.c_str(), REG_EXTENDED) != 0)
        {
          return badInput(ciP, ERROR_DESC_BAD_REQUEST_INVALID_REGEX_ENTIDPATTERN);
        }
        regfree(&re);  // If regcomp fails it frees up itself
      }
    }

    {
      Opt<std::string> typeOpt = getStringOpt(*iter, "type", "subject entities element type");

      if (!typeOpt.ok())
      {
        return badInput(ciP, typeOpt.error);
      }
      else if (typeOpt.given)
      {
        if (forbiddenIdCharsV2(typeOpt.value.c_str()))
        {
          return badInput(ciP, "forbidden characters in subject entities element type");
        }
        if (typeOpt.value.length() > MAX_ID_LEN)
        {
          return badInput(ciP, "max type length exceeded");
        }
        if (typeOpt.value.empty())
        {
          return badInput(ciP, ERROR_DESC_BAD_REQUEST_EMPTY_ENTTYPE);
        }
        type = typeOpt.value;
      }
    }

    {
      Opt<std::string> typePatOpt = getStringOpt(*iter, "typePattern", "subject entities element typePattern");

      if (!typePatOpt.ok())
      {
        return badInput(ciP, typePatOpt.error);
      }
      else if (typePatOpt.given)
      {
        if (typePatOpt.value.empty())
        {
          return badInput(ciP, "subject entities element typePattern is empty");
        }

        typePattern = typePatOpt.value;

        // FIXME P5: Keep the regex and propagate to sub-cache
        regex_t re;
        if (regcomp(&re, typePattern.c_str(), REG_EXTENDED) != 0)
        {
          return badInput(ciP, ERROR_DESC_BAD_REQUEST_INVALID_REGEX_ENTTYPEPATTERN);
        }
        regfree(&re);  // If regcomp fails it frees up itself
      }
    }


    EntID  eid(id, idPattern, type, typePattern);

    if (std::find(eivP->begin(), eivP->end(), eid) == eivP->end())  // if not already included
    {
      eivP->push_back(eid);
    }
  }

  return "";
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
      Opt<std::string> payloadOpt = getStringOpt(httpCustom, "payload", "payload httpCustom notification");

      if (!payloadOpt.ok())
      {
        return badInput(ciP, payloadOpt.error);
      }

      if (forbiddenChars(payloadOpt.value.c_str()))
      {
        return badInput(ciP, "forbidden characters in custom /payload/");
      }

      subsP->notification.httpInfo.payload = payloadOpt.value;
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

      if (r != "")
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

      if (r != "")
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
  if (notification.HasMember("attrs") && notification.HasMember("exceptAttrs"))
  {
    return badInput(ciP, "http notification has attrs and exceptAttrs");
  }

  if (notification.HasMember("attrs"))
  {
    std::string r = parseAttributeList(ciP, &subsP->notification.attributes, notification["attrs"]);

    if (r != "")
    {
      return r;
    }

    subsP->notification.blacklist = false;
    subsP->blacklistProvided      = true;
  }
  else if (notification.HasMember("exceptAttrs"))
  {
    std::string r = parseAttributeList(ciP, &subsP->notification.attributes, notification["exceptAttrs"]);

    if (r != "")
    {
      return r;
    }

    if (subsP->notification.attributes.empty())
    {
      return badInput(ciP, "http notification has exceptAttrs is empty");
    }

    subsP->notification.blacklist = true;
    subsP->blacklistProvided      = true;
  }

  // metadata
  if (notification.HasMember("metadata"))
  {
    std::string r = parseAttributeList(ciP, &subsP->notification.metadata, notification["metadata"]);

    if (r != "")
    {
      return r;
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
      return badInput(ciP, "invalid attrsFormat (accepted values: legacy, normalized, keyValues, values)");
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
    std::string r = parseAttributeList(ciP, &subsP->subject.condition.attributes, condition["attrs"]);

    if (r != "")
    {
      return r;
    }
  }

  if (condition.HasMember("expression"))
  {
    const Value& expression = condition["expression"];

    if (!expression.IsObject())
    {
      return badInput(ciP, "expression is not an object");
    }

    if (expression.ObjectEmpty())
    {
      return badInput(ciP, "expression is empty");
    }

    subsP->subject.condition.expression.isSet = true;

    if (expression.HasMember("q"))
    {
      const Value& q = expression["q"];
      std::string  qString;

      if (!q.IsString())
      {
        return badInput(ciP, "q is not a string");
      }

      qString = q.GetString();
      if (qString.empty())
      {
        return badInput(ciP, "q is empty");
      }

      subsP->subject.condition.expression.q = qString;

      std::string  errorString;
      Scope*       scopeP = new Scope(SCOPE_TYPE_SIMPLE_QUERY, qString);

      scopeP->stringFilterP = new StringFilter(SftQ);
      if (scopeP->stringFilterP->parse(scopeP->value.c_str(), &errorString) == false)
      {
        delete scopeP->stringFilterP;
        delete scopeP;

        return badInput(ciP, errorString);
      }

      subsP->restriction.scopeVector.push_back(scopeP);
    }

    if (expression.HasMember("mq"))
    {
      const Value& mq = expression["mq"];
      std::string  mqString;

      if (!mq.IsString())
      {
        return badInput(ciP, "mq is not a string");
      }

      mqString = mq.GetString();
      if (mqString.empty())
      {
        return badInput(ciP, "mq is empty");
      }

      subsP->subject.condition.expression.mq = mqString;

      std::string  errorString;
      Scope*       scopeP = new Scope(SCOPE_TYPE_SIMPLE_QUERY_MD, mqString);

      scopeP->mdStringFilterP = new StringFilter(SftMq);
      if (scopeP->mdStringFilterP->parse(scopeP->value.c_str(), &errorString) == false)
      {
        delete scopeP->mdStringFilterP;
        delete scopeP;

        return badInput(ciP, errorString);
      }

      subsP->restriction.scopeVector.push_back(scopeP);
    }

    // geometry
    int nGeoItems = 0;
    {
      Opt<std::string> geometryOpt = getStringOpt(expression, "geometry");

      if (!geometryOpt.ok())
      {
        return badInput(ciP, geometryOpt.error);
      }
      else if (geometryOpt.given)
      {
        if (geometryOpt.value.empty())
        {
          return badInput(ciP, "geometry is empty");
        }

        subsP->subject.condition.expression.geometry = geometryOpt.value;
        nGeoItems++;
      }
    }

    // coords
    {
      Opt<std::string> coordsOpt = getStringOpt(expression, "coords");

      if (!coordsOpt.ok())
      {
          return badInput(ciP, coordsOpt.error);
      }
      else if (coordsOpt.given)
      {
        if (coordsOpt.value.empty())
        {
          return badInput(ciP, "coords is empty");
        }

        subsP->subject.condition.expression.coords = coordsOpt.value;
        nGeoItems++;
      }
    }

    // georel
    {
      Opt<std::string> georelOpt = getStringOpt(expression, "georel");

      if (!georelOpt.ok())
      {
        return badInput(ciP, georelOpt.error);
      }

      if (georelOpt.given)
      {
        if (georelOpt.value.empty())
        {
          return badInput(ciP, "georel is empty");
        }

        subsP->subject.condition.expression.georel = georelOpt.value;
        nGeoItems++;
      }
    }

    if ((nGeoItems > 0) && (nGeoItems != 3))
    {
      return badInput(ciP, "partial geo expression; geometry, georel and coords have to be provided together");
    }

    //
    // If geometry, coords and georel are filled, then attempt to create a filter scope
    // with them
    //
    SubscriptionExpression subExpr = subsP->subject.condition.expression;

    if ((subExpr.georel != "") && (subExpr.geometry != "") && (subExpr.coords != ""))
    {
      Scope*       scopeP = new Scope(SCOPE_TYPE_LOCATION, "");
      std::string  err;

      if (scopeP->fill(V2, subExpr.geometry, subExpr.coords, subExpr.georel, &err) != 0)
      {
        delete scopeP;
        return badInput(ciP, "error parsing geo-query fields: " + err);
      }
      subsP->restriction.scopeVector.push_back(scopeP);
    }
  }

  return "";
}



/* ****************************************************************************
*
* parseAttributeList -
*/
static std::string parseAttributeList(ConnectionInfo* ciP, std::vector<std::string>* vec, const Value& attributes)
{
  if (!attributes.IsArray())
  {
    return badInput(ciP, "attrs is not an array");
  }

  for (Value::ConstValueIterator iter = attributes.Begin(); iter != attributes.End(); ++iter)
  {
    if (!iter->IsString())
    {
      return badInput(ciP, "attrs element is not a string");
    }

    std::string attrName = iter->GetString();

    if (attrName.empty())
    {
      return badInput(ciP, "attrs element is empty");
    }

    if (forbiddenIdCharsV2(attrName.c_str()))
    {
      return badInput(ciP, "attrs element has forbidden char");
    }

    if (attrName.length() > MAX_ID_LEN)
    {
      return badInput(ciP, "max attribute length exceeded");
    }

    vec->push_back(attrName);
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



/* ****************************************************************************
*
* badInput -
*/
static std::string badInput(ConnectionInfo* ciP, const std::string& msg)
{
  alarmMgr.badInput(clientIp, msg);
  OrionError oe(SccBadRequest, msg, "BadRequest");

  ciP->httpStatusCode = oe.code;

  return oe.toJson();
}
