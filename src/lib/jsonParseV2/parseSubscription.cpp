#include "parseSubscription.h"
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

#include "common/globals.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "jsonParseV2/jsonParseTypeNames.h"

#include "jsonParseV2/parseSubscription.h"

using namespace rapidjson;


/* Prototypes */
static std::string parseAttributeList(ConnectionInfo* ciP, std::vector<std::string>* vec, const Value& attributes);
static std::string parseNotification(ConnectionInfo* ciP, SubscribeContextRequest* scrP, const Value& notification);
static std::string parseSubject(ConnectionInfo* ciP, SubscribeContextRequest* scrP, const Value& subject);
static std::string parseEntitiesVector(ConnectionInfo* ciP, EntityIdVector* eivP, const Value& entities);
static std::string parseNotifyConditionVector(ConnectionInfo* ciP, NotifyConditionVector* ncvP, const Value& condition);


/* ****************************************************************************
*
* parseSubscription -
*
*/
std::string parseSubscription(ConnectionInfo* ciP, ParseData* parseDataP)
{
  Document    document;
  std::string r;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    LM_W(("Bad Input (JSON parse error)"));
    OrionError oe(SccBadRequest, "Errors found in incoming JSON buffer");

    return oe.render(ciP, "");
  }

  if (!document.IsObject())
  {
    LM_E(("Bad Input (JSON Parse Error)"));
    OrionError oe(SccBadRequest, "Error parsing incoming JSON buffer");

    return oe.render(ciP, "");
  }

  if (document.ObjectEmpty())
  {
    //
    // Initially we used the method "Empty". As the broker crashed inside that method, some
    // research was made and "ObjectEmpty" was found. As the broker stopped crashing and complaints
    // about crashes with small docs and "Empty()" were found on the internet, we opted to use ObjectEmpty
    //
    LM_W(("Bad Input (Empty payload)"));
    OrionError oe(SccBadRequest, "empty payload");

    return oe.render(ciP, "");
  }

  if (!document.HasMember("subject"))
  {
    LM_W(("Bad Input (No subject specified"));
    OrionError oe(SccBadRequest, "no subject for subscription specified");

    return oe.render(ciP, "");
  }

  if (!document.HasMember("notification"))
  {
    LM_W(("Bad Input (No notification specified"));
    OrionError oe(SccBadRequest, "no notitifcation for subscription specified");

    return oe.render(ciP, "");
  }

  if (!document.HasMember("expires"))
  {
    LM_W(("Bad Input (No expires specified"));
    OrionError oe(SccBadRequest, "no expiration for subscription specified");

    return oe.render(ciP, "");
  }

  const Value& expires = document["expires"];

  if (!expires.IsString())
  {
    LM_W(("Bad Input (Expires is not an string"));
    OrionError oe(SccBadRequest, "expires is not an string");

    return oe.render(ciP, "");
  }

  int64_t eT= parse8601Time(expires.GetString());

  if (eT == -1)
  {
    LM_W(("Bad Input (Expires has an invalid format"));
    OrionError oe(SccBadRequest, "expires has a invalid format");

    return oe.render(ciP, "");
  }
  parseDataP->scr.res.expires = eT;

  const Value& notification = document["notification"];
  r = parseNotification(ciP, &parseDataP->scr.res, notification);
  if (r != "") {
    return r;
  }

  const Value& subject = document["subject"];
  r = parseSubject(ciP, &parseDataP->scr.res, subject);
  if (r != "") {
    return r;
  }

  return "OK";
}



/* ****************************************************************************
*
* parseSubject -
*
*/

static std::string parseSubject(ConnectionInfo* ciP, SubscribeContextRequest* scrP, const Value& subject)
{

  //parseDataP->scr.res.entityIdVector = NULL;
  //parseDataP->scr.res.notifyConditionVector = NULL;
  std::string r;

  // Entities
  if (!subject.HasMember("entities"))
  {
    LM_W(("Bad Input (No subject entities specified"));
    OrionError oe(SccBadRequest, "no subject entities specified");

    return oe.render(ciP, "");
  }
  else
  {
    r = parseEntitiesVector(ciP, &scrP->entityIdVector, subject["entities"]);
    if (r != "") {
      return r;
    }
  }

  //Condition
  if (!subject.HasMember("condition"))
  {
    LM_W(("Bad Input (No subject condition specified"));
    OrionError oe(SccBadRequest, "no subject condition specified");

    return oe.render(ciP, "");
  }
  else
  {
    r = parseNotifyConditionVector(ciP, &scrP->notifyConditionVector, subject["condition"]);
    if (r != "") {
      return r;
    }
  }
  return "";
}



/* ****************************************************************************
*
* parseEntitiesVector -
*
*/

static std::string parseEntitiesVector(ConnectionInfo* ciP, EntityIdVector* eivP, const Value& entities)
{
  if (!entities.IsArray())
  {
    LM_W(("Bad Input (subject entities is not an array"));
    OrionError oe(SccBadRequest, "subject entities is not an array");

    return oe.render(ciP, "");
  }
  for (Value::ConstValueIterator iter = entities.Begin(); iter != entities.End(); ++iter)
  {
    if (!iter->IsObject())
    {
      LM_W(("Bad Input (subject entities element is not an object"));
      OrionError oe(SccBadRequest, "subject entities element is not an object");

      return oe.render(ciP, "");
    }
    if (!iter->HasMember("id") && !iter->HasMember("idPattern") && !iter->HasMember("type"))
    {
      LM_W(("Bad Input (subject entities element has not id/idPattern nor type"));
      OrionError oe(SccBadRequest, "subject entities element has not id/idPattern");

      return oe.render(ciP, "");
    }

    if (iter->HasMember("id") && iter->HasMember("idPattern"))
    {
      LM_W(("Bad Input (subject entities element has id and idPattern"));
      OrionError oe(SccBadRequest, "subject entities element has id and idPattern");

      return oe.render(ciP, "");
    }
    EntityId* eiP         = new EntityId();
    std::string isPattern = "false";
    std::string id;

    if (iter->HasMember("id"))
    {
      if ((*iter)["id"].IsString())
      {
        id = (*iter)["id"].GetString();
      }
      else
      {
        LM_W(("Bad Input (subject entities element id is not a string"));
        OrionError oe(SccBadRequest, "subject entities element id is not a string");

        return oe.render(ciP, "");
      }
    }
    if (iter->HasMember("idPattern"))
    {
      if ((*iter)["idPattern"].IsString())
      {
        id = (*iter)["id"].GetString();
      }
      else
      {
        LM_W(("Bad Input (subject entities element idPattern is not a string"));
        OrionError oe(SccBadRequest, "subject entities element idPattern is not a string");

        return oe.render(ciP, "");
      }
      isPattern = "true";
    }
    if (id == "")  // Only type was provided
    {
      id = ".*";
      isPattern = "true";
    }
    eiP->id = id;
    eiP->isPattern = isPattern;

    if (iter->HasMember("type"))
    {
      if ((*iter)["type"].IsString())
      {
        id = (*iter)["type"].GetString();
      }
      else
      {
        LM_W(("Bad Input (subject entities element type is not a string"));
        OrionError oe(SccBadRequest, "subject entities element type is not a string");

        return oe.render(ciP, "");
      }
    }

    eivP->push_back_if_absent(eiP);
  }
  return "";

}



/* ****************************************************************************
*
* parseNotification -
*
*/

static std::string parseNotification(ConnectionInfo* ciP, SubscribeContextRequest* scrP, const Value& notification)
{
  // Callback
  if (notification.HasMember("callback"))
  {
    const Value& callback = notification["callback"];
    if (!callback.IsString())
    {
      LM_W(("Bad Input (callback is not an string"));
      OrionError oe(SccBadRequest, "callback is not an string");

      return oe.render(ciP, "");
    }
    scrP->reference.string = callback.GetString();
  }
  else // missing callback field
  {
    LM_W(("Bad Input (callback is missing"));
    OrionError oe(SccBadRequest, "callback is missing");

    return oe.render(ciP, "");
  }

  // Attributes
  if (!notification.HasMember("attributes"))
  {
    LM_W(("Bad Input (No notification attributes specified"));
    OrionError oe(SccBadRequest, "no notification attributes specified");

    return oe.render(ciP, "");
  }
  else
  {
    std::string r = parseAttributeList(ciP, &scrP->attributeList.attributeV, notification["attributes"]);

    if (r != "") {
      return r;
    }
  }

  // Throttling
  if (notification.HasMember("throttling"))
  {
    const Value& throttling = notification["throttling"];
    if (!throttling.IsInt64())
    {
      LM_W(("Bad Input (Throttling is not a int"));
      OrionError oe(SccBadRequest, "throttling is not an int");

      return oe.render(ciP, "");
    }
    scrP->throttling.seconds = throttling.GetInt64();
  }
  return "";
}



/* ****************************************************************************
*
* parseNotifyConditionVector -
*
*/

static std::string parseNotifyConditionVector(ConnectionInfo* ciP, NotifyConditionVector* ncvP, const Value& condition)
{
  // Attributes
  if (!condition.HasMember("attributes"))
  {
    LM_W(("Bad Input (No condition attributes specified"));
    OrionError oe(SccBadRequest, "no condition attributes specified");

    return oe.render(ciP, "");
  }
  NotifyCondition* nc = new NotifyCondition();
  nc->type = ON_CHANGE_CONDITION;
  ncvP->push_back(nc);

  std::string r = parseAttributeList(ciP, &nc->condValueList.vec, condition["attributes"]);

  if (r != "") {
    return r;
  }

  if (condition.HasMember("expression"))
  {
    const Value& expression = condition["expression"];
    if (expression.HasMember("q"))
    {
      nc->expression.q = expression["q"].GetString();
    }
    if (expression.HasMember("geometry"))
    {
      nc->expression.geometry = expression["geometry"].GetString();
    }
    if (expression.HasMember("coords"))
    {
      nc->expression.coords = expression["coords"].GetString();
    }
    if (expression.HasMember("georel"))
    {
      nc->expression.georel = expression["georel"].GetString();
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
  std::set<std::string> s;

  if (!attributes.IsArray())
  {
    LM_W(("Bad Input (attributes is not an array"));
    OrionError oe(SccBadRequest, "attributes is not an array");

    return oe.render(ciP, "");
  }
  for (Value::ConstValueIterator iter = attributes.Begin(); iter != attributes.End(); ++iter)
  {
    if (!iter->IsString())
    {
      LM_W(("Bad Input (attributes element is not an string"));
      OrionError oe(SccBadRequest, "attributes element is not an string");

      return oe.render(ciP, "");
    }
    s.insert(iter->GetString());
  }
  vec->resize(s.size());
  copy(s.begin(), s.end(),vec->begin());
  return "";
}
