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

#include <algorithm>

#include "rapidjson/document.h"

#include "alarmMgr/alarmMgr.h"
#include "common/globals.h"
#include "common/RenderFormat.h"
#include "common/string.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "ngsi/Request.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/jsonRequestTreat.h"
#include "jsonParseV2/utilsParse.h"
#include "rest/Verb.h"


#include "jsonParseV2/parseSubscription.h"

using namespace rapidjson;

using ngsiv2::SubscriptionUpdate;
using ngsiv2::EntID;

/* Prototypes */
static std::string parseAttributeList(ConnectionInfo* ciP, std::vector<std::string>* vec, const Value& attributes);
static std::string parseNotification(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& notification);
static std::string parseSubject(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& subject);
static std::string parseEntitiesVector(ConnectionInfo* ciP, std::vector<EntID>* eivP, const Value& entities);
static std::string parseNotifyConditionVector(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& condition);
static std::string error(ConnectionInfo* ciP, const std::string& msg);


/* ****************************************************************************
*
* parseSubscription -
*
*/
std::string parseSubscription(ConnectionInfo* ciP, SubscriptionUpdate* subsP, bool update)
{
  Document document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    OrionError oe(SccBadRequest, "Errors found in incoming JSON buffer", ERROR_STRING_PARSERROR);
    alarmMgr.badInput(clientIp, "JSON parse error");
    return oe.render(ciP, "");
  }

  if (!document.IsObject())
  {
    OrionError oe(SccBadRequest, "Error parsing incoming JSON buffer", ERROR_STRING_PARSERROR);
    alarmMgr.badInput(clientIp, "JSON parse error");
    return oe.render(ciP, "");
  }

  if (document.ObjectEmpty())
  {
    //
    // Initially we used the method "Empty". As the broker crashed inside that method, some
    // research was made and "ObjectEmpty" was found. As the broker stopped crashing and complaints
    // about crashes with small docs and "Empty()" were found on the internet, we opted to use ObjectEmpty
    //
    return error(ciP, "empty payload");
  }


  // Description field

  Opt<std::string> description = getStringOpt(document, "description");
  if (!description.ok())
  {
    return description.error;
  }
  else if (description.given)
  {
    std::string descriptionString = description.value;

    if (descriptionString.length() > MAX_DESCRIPTION_LENGTH)
    {
      return error(ciP, "max description length exceeded");
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
    return error(ciP, "no subject for subscription specified");
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
    return error(ciP, "no notification for subscription specified");
  }


  // Expires field
  Opt<std::string> expiresOpt = getStringOpt(document, "expires");

  if (!expiresOpt.ok())
  {
    return expiresOpt.error;
  }
  else if (expiresOpt.given)
  {
    std::string expires = expiresOpt.value;

    int64_t eT = -1;

    if (expires.empty())
    {
        eT = PERMANENT_SUBS_DATETIME;
    }
    else
    {
      eT = parse8601Time(expires);
      if (eT == -1)
      {
        return error(ciP, "expires has an invalid format");
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
  Opt<std::string> statusOpt =  getStringOpt(document, "status");
  if (!statusOpt.ok())
  {
    return statusOpt.error;
  }
  else if (statusOpt.given)
  {
    std::string statusString = statusOpt.value;

    if ((statusString != "active") && (statusString != "inactive"))
    {
      return error(ciP, "status is not valid (it has to be either active or inactive)");
    }
    subsP->statusProvided = true;
    subsP->status = statusString;
  }


  // Throttling
  Opt<int64_t> throttlingOpt = getInt64Opt(document, "throttling");
  if (!throttlingOpt.ok())
  {
    return throttlingOpt.error;
  }
  else if (throttlingOpt.given)
  {
    subsP->throttlingProvided = true;
    subsP->throttling = throttlingOpt.value;
  }
  else if (!update) // throttling was not set and it is not update
  {
    subsP->throttling = 0; // Default value if not provided at creation => no throttling
  }


  // attrsFormat field
  Opt<std::string>  attrsFormatOpt = getStringOpt(document, "attrsFormat");
  if (!attrsFormatOpt.ok())
  {
    return attrsFormatOpt.error;
  }
  else if (attrsFormatOpt.given)
  {
    std::string   attrsFormatString = attrsFormatOpt.value;
    RenderFormat  nFormat           = stringToRenderFormat(attrsFormatString, true);

    if (nFormat == NO_FORMAT)
    {
      return error(ciP, "invalid attrsFormat (accepted values: legacy, normalized, keyValues, values)");
    }
    subsP->attrsFormatProvided = true;
    subsP->attrsFormat = nFormat;
  }
  else if (!update) // Default value for creation
  {
    subsP->attrsFormat = DEFAULT_RENDER_FORMAT;  // Default format for NGSIv2: normalized
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
    return error(ciP, "subject is not an object");
  }

  // Entities
  if (!subject.HasMember("entities"))
  {
    return error(ciP, "no subject entities specified");
  }

  r = parseEntitiesVector(ciP, &subsP->subject.entities, subject["entities"] );
  if (r != "")
  {
    return r;
  }


  // Condition
  if (!subject.HasMember("condition"))
  {
    return error(ciP, "no subject condition specified");
  }

  const Value& condition = subject["condition"];
  if (!condition.IsObject())
  {
    return error(ciP, "condition is not an object");
  }
  r = parseNotifyConditionVector(ciP, subsP, condition);

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
    return error(ciP, "subject entities is not an array");
  }

  for (Value::ConstValueIterator iter = entities.Begin(); iter != entities.End(); ++iter)
  {
    if (!iter->IsObject())
    {
     return error(ciP, "subject entities element is not an object");
    }
    if (!iter->HasMember("id") && !iter->HasMember("idPattern") && !iter->HasMember("type"))
    {
      return error(ciP, "subject entities element has not id/idPattern nor type");
    }

    if (iter->HasMember("id") && iter->HasMember("idPattern"))
    {
      return error(ciP, "subject entities element has id and idPattern");
    }

    std::string  id;
    std::string  idPattern;
    std::string  type;

    {
      Opt<std::string> idOpt = getStringOpt(*iter,"id", "subject entities element id");
      if (!idOpt.ok())
      {
        return idOpt.error;
      }
      id = idOpt.value;

      if (id.empty())
      {
        idPattern = ".*";
      }
    }

    {
      Opt<std::string> idPatOpt = getStringOpt(*iter,"idPattern", "subject entities element idPattern");
      if (!idPatOpt.ok())
      {
        return idPatOpt.error;
      }
      else if (idPatOpt.given)
      {
        idPattern = idPatOpt.value;
      }
    }

    Opt<std::string> typeOpt = getStringOpt(*iter, "type", "subject entities element type");
    if (!typeOpt.ok())
    {
      return typeOpt.error;
    }
    else if (typeOpt.given)
    {
      type = typeOpt.value;
    }

    EntID  eid(id, idPattern, type);

    if (std::find(eivP->begin(), eivP->end(), eid) == eivP->end()) // if not already included
    {
      eivP->push_back(eid);
    }
  }

  return "";
}



/* ****************************************************************************
*
* parseNotification -
*
*/
static std::string parseNotification(ConnectionInfo* ciP, SubscriptionUpdate* subsP, const Value& notification)
{

  subsP->notificationProvided = true;

  if (!notification.IsObject())
  {
    return error(ciP, "notitication is not an object");
  }


/*
 *
 * XOR de http/httpExtended. En funcion de como se defina
 *  Ver si tiene sentido chequear la URL, si es que se puede
 * templatizar en la llamada tradicional
 *
 *
 */



  // Callback
  if (notification.HasMember("http"))
  {
    const Value& http = notification["http"];
    if (!http.IsObject())
    {
      return error(ciP, "http notitication is not an object");
    }

    // http - url
    std::string url = getString(http, "url", "url http notification");

    {
       std::string  host;
       int          port;
       std::string  path;
       std::string  protocol;

       if (parseUrl(url, host, port, path, protocol) == false)
       {
          return error(ciP, "Invalid URL parsing notification url");
       }
    }

    subsP->notification.httpInfo.url =      url;
    subsP->notification.httpInfo.extended = false;

  }
  else if (notification.HasMember("httpExtended"))
  {
    const Value& httpExt = notification["httpExtended"];
    if (!httpExt.IsObject())
    {
      return error(ciP, "httpExtended notitication is not an object");
    }

    // URL
    std::string url = getString(httpExt, "url", "url http notification");

    subsP->notification.httpInfo.url = url;


    // method -> verb
    std::string method = getString(httpExt, "method", "method http notification");
    Verb  verb         = str2Verb(method);
    /*
     *  CHECK IT IS VALID
     */

    subsP->notification.httpInfo.verb = verb;

    // payload
    std::string payload = getString(httpExt, "payload", "payload http notification");
    subsP->notification.httpInfo.payload = payload;


    subsP->notification.httpInfo.extended = true;

  }
  else  // missing callback field
  {
    return error(ciP, "http notification is missing");
  }


  // Attributes
  if (notification.HasMember("attrs"))
  {
    std::string r = parseAttributeList(ciP, &subsP->notification.attributes, notification["attrs"]);

    if (r != "")
    {
      return r;
    }
  }

   return "";
}



/* ****************************************************************************
*
* parseNotifyConditionVector -
*
*/
static std::string parseNotifyConditionVector(ConnectionInfo* ciP, ngsiv2::SubscriptionUpdate* subsP, const Value& condition)
{


  if (!condition.IsObject())
  {
   return error(ciP, "condition is not an object");
  }
  // Attributes
  if (!condition.HasMember("attrs"))
  {
    return error(ciP, "no condition attrs specified");
  }

  std::string r = parseAttributeList(ciP, &subsP->subject.condition.attributes, condition["attrs"]);

  if (r != "")
  {
    return r;
  }

  if (condition.HasMember("expression"))
  {
    const Value& expression = condition["expression"];

    if (!expression.IsObject())
    {
      return error(ciP, "expression is not an object");
    }

    subsP->subject.condition.expression.isSet = true;

    if (expression.HasMember("q"))
    {
      const Value& q = expression["q"];
      if (!q.IsString())
      {
        return error(ciP, "q is not a string");
      }
      subsP->subject.condition.expression.q = q.GetString();

      std::string  errorString;
      Scope*       scopeP = new Scope(SCOPE_TYPE_SIMPLE_QUERY, expression["q"].GetString());

      scopeP->stringFilterP = new StringFilter();
      if (scopeP->stringFilterP->parse(scopeP->value.c_str(), &errorString) == false)
      {
        delete scopeP->stringFilterP;
        delete scopeP;

        return error(ciP, errorString);

      }

      subsP->restriction.scopeVector.push_back(scopeP);
    }

    // geometry
    Opt<std::string> geometryOpt = getStringOpt(expression, "geometry");

    if (!geometryOpt.ok())
    {
      return geometryOpt.error;
    }
    else if (geometryOpt.given)
    {
      subsP->subject.condition.expression.geometry = geometryOpt.value;
    }

    // coords
    Opt<std::string> coordsOpt = getStringOpt(expression, "coords");

    if (!coordsOpt.ok())
    {
        return coordsOpt.error;
    }
    else if (coordsOpt.given)
    {
      subsP->subject.condition.expression.coords = coordsOpt.value;
    }

    // georel
    Opt<std::string> georelOpt = getStringOpt(expression, "georel");
    if (!geometryOpt.ok())
    {
      return geometryOpt.error;
    }
    if (georelOpt.given)
    {
      subsP->subject.condition.expression.georel = georelOpt.value;
    }
  }
  return "";
}



/* ****************************************************************************
*
* parseAttributeList -
*
*/
static std::string parseAttributeList(ConnectionInfo* ciP, std::vector<std::string>* vec, const Value& attributes)
{
  if (!attributes.IsArray())
  {
    return error(ciP, "attrs is not an array");
  }

  for (Value::ConstValueIterator iter = attributes.Begin(); iter != attributes.End(); ++iter)
  {
    if (!iter->IsString())
    {
      return error(ciP, "attrs element is not an string");
    }

    vec->push_back(iter->GetString());
  }

  return "";
}

/* ****************************************************************************
*
* error -
*
*/
static std::string error(ConnectionInfo* ciP, const std::string& msg)
{
  alarmMgr.badInput(clientIp, msg);
  OrionError oe(SccBadRequest, msg);

  return oe.render(ciP, "");
}

