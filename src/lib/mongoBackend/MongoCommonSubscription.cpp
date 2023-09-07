/*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*
*/
#include <string>
#include <vector>
#include <map>

#include "mongoBackend/MongoCommonSubscription.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/defaultValues.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/compoundValueBson.h"

#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/BSONArrayBuilder.h"


/* ****************************************************************************
*
* USING - 
*/
using ngsiv2::Subscription;
using ngsiv2::HttpInfo;
using ngsiv2::EntID;



/* ****************************************************************************
*
* setNewSubscriptionId -
*/
std::string setNewSubscriptionId(orion::BSONObjBuilder* b)
{
  orion::OID  oid;

  oid.init();
  b->append("_id", oid);

  LM_T(LmtMongo, ("Subscription _id: %s", oid.toString().c_str()));
  return oid.toString();
}



/* ****************************************************************************
*
* setExpiration -
*/
void setExpiration(const Subscription& sub, orion::BSONObjBuilder* b)
{
  b->append(CSUB_EXPIRATION, sub.expires);
  LM_T(LmtMongo, ("Subscription expiration: %lu", sub.expires));
}



/* ****************************************************************************
*
* setCustomHttpInfo -
*/
static void setCustomHttpInfo(const HttpInfo& httpInfo, orion::BSONObjBuilder* b)
{
  if (httpInfo.verb != NOVERB)
  {
    std::string method = verbName(httpInfo.verb);

    b->append(CSUB_METHOD, method);
    LM_T(LmtMongo, ("Subscription method: %s", method.c_str()));
  }

  if (httpInfo.headers.size() > 0)
  {
    orion::BSONObjBuilder headersBuilder;

    for (std::map<std::string, std::string>::const_iterator it = httpInfo.headers.begin(); it != httpInfo.headers.end(); ++it)
    {
      headersBuilder.append(it->first, it->second);
    }
    orion::BSONObj headersObj = headersBuilder.obj();

    b->append(CSUB_HEADERS, headersObj);
    LM_T(LmtMongo, ("Subscription headers: %s", headersObj.toString().c_str()));
  }

  if (httpInfo.qs.size() > 0)
  {
    orion::BSONObjBuilder qsBuilder;

    for (std::map<std::string, std::string>::const_iterator it = httpInfo.qs.begin(); it != httpInfo.qs.end(); ++it)
    {
      qsBuilder.append(it->first, it->second);
    }

    orion::BSONObj qsObj = qsBuilder.obj();

    b->append(CSUB_QS, qsObj);
    LM_T(LmtMongo, ("Subscription qs: %s", qsObj.toString().c_str()));
  }

  if (httpInfo.payloadType == ngsiv2::CustomPayloadType::Text)
  {
    if (!httpInfo.includePayload)
    {
      b->appendNull(CSUB_PAYLOAD);
      LM_T(LmtMongo, ("Subscription payload: null"));
    }
    else if (!httpInfo.payload.empty())
    {
      b->append(CSUB_PAYLOAD, httpInfo.payload);
      LM_T(LmtMongo, ("Subscription payload: %s", httpInfo.payload.c_str()));
    }
  }
  else if (httpInfo.payloadType == ngsiv2::CustomPayloadType::Json)
  {
    if (httpInfo.json != NULL)
    {
      std::string logStr;
      if (httpInfo.json->isObject())
      {
        orion::BSONObjBuilder jsonBuilder;
        compoundValueBson(httpInfo.json->childV, jsonBuilder, false);
        orion::BSONObj jsonBuilderObj = jsonBuilder.obj();
        logStr = jsonBuilderObj.toString();
        b->append(CSUB_JSON, jsonBuilderObj);
      }
      else  // httpInfo.json->isVector();
      {
        orion::BSONArrayBuilder jsonBuilder;
        compoundValueBson(httpInfo.json->childV, jsonBuilder, false);
        orion::BSONArray jsonBuilderArr = jsonBuilder.arr();
        logStr = jsonBuilderArr.toString();
        b->append(CSUB_JSON, jsonBuilderArr);
      }
      LM_T(LmtMongo, ("Subscription json: %s", logStr.c_str()));
    }
  }
  else  // httpInfo.payloadType == ngsiv2::CustomPayloadType::Ngsi
  {
    // id and type (both optional in this case)
    orion::BSONObjBuilder bob;
    if (!httpInfo.ngsi.id.empty())
    {
      bob.append(ENT_ENTITY_ID, httpInfo.ngsi.id);
    }
    if (!httpInfo.ngsi.type.empty())
    {
      bob.append(ENT_ENTITY_TYPE, httpInfo.ngsi.type);
    }

    // attributes
    // (-1 as date as creDate and modDate are not used in this case)
    orion::BSONObjBuilder    attrsToAdd;  // not actually used
    orion::BSONArrayBuilder  attrNamesToAdd;
    httpInfo.ngsi.attributeVector.toBson(-1, &attrsToAdd, &attrNamesToAdd, V2);

    // note that although metadata is not needed in the ngsi field logic,
    // mdNames: [ ] is added to each attribute as a consequence of the toBson() logic
    bob.append(ENT_ATTRS, attrsToAdd.obj());

    b->append(CSUB_NGSI, bob.obj());
  }
}



/* ****************************************************************************
*
* setCustomMqttInfo -
*/
static void setCustomMqttInfo(const ngsiv2::MqttInfo& mqttInfo, orion::BSONObjBuilder* b)
{
  if (mqttInfo.payloadType == ngsiv2::CustomPayloadType::Text)
  {
    if (!mqttInfo.includePayload)
    {
      b->appendNull(CSUB_PAYLOAD);
      LM_T(LmtMongo, ("Subscription payload: null"));
    }
    else if (!mqttInfo.payload.empty())
    {
      b->append(CSUB_PAYLOAD, mqttInfo.payload);
      LM_T(LmtMongo, ("Subscription payload: %s", mqttInfo.payload.c_str()));
    }
  }
  else if (mqttInfo.payloadType == ngsiv2::CustomPayloadType::Json)
  {
    if (mqttInfo.json != NULL)
    {
      std::string logStr;
      if (mqttInfo.json->isObject())
      {
        orion::BSONObjBuilder jsonBuilder;
        compoundValueBson(mqttInfo.json->childV, jsonBuilder, false);
        orion::BSONObj jsonBuilderObj = jsonBuilder.obj();
        logStr = jsonBuilderObj.toString();
        b->append(CSUB_JSON, jsonBuilderObj);
      }
      else  // httpInfo.json->isVector();
      {
        orion::BSONArrayBuilder jsonBuilder;
        compoundValueBson(mqttInfo.json->childV, jsonBuilder, false);
        orion::BSONArray jsonBuilderArr = jsonBuilder.arr();
        logStr = jsonBuilderArr.toString();
        b->append(CSUB_JSON, jsonBuilderArr);
      }
      LM_T(LmtMongo, ("Subscription json: %s", logStr.c_str()));
    }
  }
  else  // mqttInfo.payloadType == ngsiv2::CustomPayloadType::Ngsi
  {
    // id and type (both optional in this case)
    orion::BSONObjBuilder bob;
    if (!mqttInfo.ngsi.id.empty())
    {
      bob.append(ENT_ENTITY_ID, mqttInfo.ngsi.id);
    }
    if (!mqttInfo.ngsi.type.empty())
    {
      bob.append(ENT_ENTITY_TYPE, mqttInfo.ngsi.type);
    }

    // attributes
    // (-1 as date as creDate and modDate are not used in this case)
    orion::BSONObjBuilder    attrsToAdd;  // not actually used
    orion::BSONArrayBuilder  attrNamesToAdd;
    mqttInfo.ngsi.attributeVector.toBson(-1, &attrsToAdd, &attrNamesToAdd, V2);

    // note that although metadata is not needed in the ngsi field logic,
    // mdNames: [ ] is added to each attribute as a consequence of the toBson() logic
    bob.append(ENT_ATTRS, attrsToAdd.obj());

    b->append(CSUB_NGSI, bob.obj());
  }
}



/* ****************************************************************************
*
* setNotificationInfo -
*/
void setNotificationInfo(const Subscription& sub, orion::BSONObjBuilder* b)
{
  if (sub.notification.type == ngsiv2::HttpNotification)
  {
    b->append(CSUB_REFERENCE,     sub.notification.httpInfo.url);
    b->append(CSUB_CUSTOM,        sub.notification.httpInfo.custom);
    b->append(CSUB_TIMEOUT,   sub.notification.httpInfo.timeout);

    LM_T(LmtMongo, ("Subscription reference:   %s", sub.notification.httpInfo.url.c_str()));
    LM_T(LmtMongo, ("Subscription custom:      %s", sub.notification.httpInfo.custom? "true" : "false"));
    LM_T(LmtMongo, ("Subscription timeout: %d", sub.notification.httpInfo.timeout));


    if (sub.notification.httpInfo.custom)
    {
      setCustomHttpInfo(sub.notification.httpInfo, b);
    }
  }
  else  // MqttNotification
  {
    b->append(CSUB_REFERENCE, sub.notification.mqttInfo.url);
    b->append(CSUB_MQTTTOPIC, sub.notification.mqttInfo.topic);
    b->append(CSUB_MQTTQOS,   (int) sub.notification.mqttInfo.qos);
    b->append(CSUB_MQTTRETAIN, sub.notification.mqttInfo.retain);
    b->append(CSUB_CUSTOM,    sub.notification.mqttInfo.custom);

    LM_T(LmtMongo, ("Subscription reference:  %s", sub.notification.mqttInfo.url.c_str()));
    LM_T(LmtMongo, ("Subscription mqttTopic:  %s", sub.notification.mqttInfo.topic.c_str()));
    LM_T(LmtMongo, ("Subscription mqttQos:    %d", sub.notification.mqttInfo.qos));
    LM_T(LmtMongo, ("Subscription mqttRetain: %s", sub.notification.mqttInfo.retain? "true": "false"));
    LM_T(LmtMongo, ("Subscription custom:     %s", sub.notification.mqttInfo.custom? "true" : "false"));

    if (sub.notification.mqttInfo.providedAuth)
    {
      b->append(CSUB_USER,   sub.notification.mqttInfo.user);
      b->append(CSUB_PASSWD, sub.notification.mqttInfo.passwd);
      LM_T(LmtMongo, ("Subscription user:   %s", sub.notification.mqttInfo.user.c_str()));
      LM_T(LmtMongo, ("Subscription passwd: *****"));
    }

    if (sub.notification.mqttInfo.custom)
    {
      setCustomMqttInfo(sub.notification.mqttInfo, b);
    }
  }
}



/* ****************************************************************************
*
* setThrottling -
*/
void setThrottling(const Subscription& sub, orion::BSONObjBuilder* b)
{
  b->append(CSUB_THROTTLING, sub.throttling);
  LM_T(LmtMongo, ("Subscription throttling: %lu", sub.throttling));
}



/* ****************************************************************************
*
* setMaxfailsLimit -
*/
void setMaxFailsLimit(const Subscription& sub, orion::BSONObjBuilder* b)
{
  b->append(CSUB_MAXFAILSLIMIT, sub.notification.maxFailsLimit);
  LM_T(LmtMongo, ("Subscription maxFailsLimit: %lu", sub.notification.maxFailsLimit));
}



/* ****************************************************************************
*
* setFailsCounter -
*/
void setFailsCounter(long long failedCounter, orion::BSONObjBuilder* b)
{
  b->append(CSUB_FAILSCOUNTER, failedCounter);
  LM_T(LmtMongo, ("Subscription failsCounter: %lu", failedCounter));
}



/* ****************************************************************************
*
* setServicePath -
*/
void setServicePath(const std::string& servicePath, orion::BSONObjBuilder* b)
{
  b->append(CSUB_SERVICE_PATH, servicePath);
  LM_T(LmtMongo, ("Subscription servicePath: %s", servicePath.c_str()));
}



/* ****************************************************************************
*
* setDescription -
*/
void setDescription(const Subscription& sub, orion::BSONObjBuilder* b)
{
  b->append(CSUB_DESCRIPTION, sub.description);
  LM_T(LmtMongo, ("Subscription description: %s", sub.description.c_str()));
}



/* ****************************************************************************
*
* setStatus -
*/
void setStatus(const std::string& _status, orion::BSONObjBuilder* b, double now)
{
  std::string  status = (_status.empty())? STATUS_ACTIVE : _status;

  b->append(CSUB_STATUS, status);
  b->append(CSUB_STATUS_LAST_CHANGE, now);
  LM_T(LmtMongo, ("Subscription status: %s", status.c_str()));
  LM_T(LmtMongo, ("Subscription status last change: %f", now));
}



/* ****************************************************************************
*
* setEntities -
*/
void setEntities(const Subscription& sub, orion::BSONObjBuilder* b, bool fromNgsiv1)
{
  orion::BSONArrayBuilder entities;

  for (unsigned int ix = 0; ix < sub.subject.entities.size(); ++ix)
  {
    EntID       en            = sub.subject.entities[ix];
    std::string finalId;
    std::string finalType;
    std::string isIdPattern;
    bool        isTypePattern = false;

    //
    // Note that, due to legacy reasons, isPattern may be "true" or "false" (text)
    // while isTypePattern may be true or false (boolean).
    //
    if (!en.idPattern.empty())
    {
      finalId     = en.idPattern;
      isIdPattern = "true";
    }
    else if (!en.id.empty())
    {
      finalId     = en.id;
      isIdPattern = "false";
    }

    if (!en.typePattern.empty())
    {
      finalType     = en.typePattern;
      isTypePattern = true;
    }
    else if (!en.type.empty())
    {
      finalType     = en.type;
      isTypePattern = false;
    }

    orion::BSONObjBuilder bob;
    bob.append(CSUB_ENTITY_ID, finalId);
    bob.append(CSUB_ENTITY_ISPATTERN, isIdPattern);
    if (!finalType.empty())
    {
      bob.append(CSUB_ENTITY_TYPE, finalType);
      bob.append(CSUB_ENTITY_ISTYPEPATTERN, isTypePattern);
    }
    entities.append(bob.obj());
  }

  if ((fromNgsiv1) && (entities.arrSize() == 0))
  {
    // Special case: in NGSIv1 entities and condition attributes are not
    // part of the same field (subject, in NGSIv2) so it may happen that
    // subject only contains condition attributes and entities has to be
    // left untouched in this case
    return;
  }

  orion::BSONArray entitiesArr = entities.arr();

  b->append(CSUB_ENTITIES, entitiesArr);
  LM_T(LmtMongo, ("Subscription entities: %s", entitiesArr.toString().c_str()));
}



/* ****************************************************************************
*
* setAttrs -
*/
void setAttrs(const Subscription& sub, orion::BSONObjBuilder* b)
{
  orion::BSONArrayBuilder attrs;

  for (unsigned int ix = 0; ix < sub.notification.attributes.size(); ++ix)
  {
    attrs.append(sub.notification.attributes[ix]);
  }

  orion::BSONArray attrsArr = attrs.arr();
  b->append(CSUB_ATTRS, attrsArr);
  LM_T(LmtMongo, ("Subscription attributes: %s", attrsArr.toString().c_str()));
}



/* ****************************************************************************
*
* setConds
*/
void setConds
(
  const Subscription&     sub,
  orion::BSONObjBuilder*  b
)
{
  orion::BSONArrayBuilder conds;

  for (unsigned int ix = 0; ix < sub.subject.condition.attributes.size(); ++ix)
  {
    conds.append(sub.subject.condition.attributes[ix]);
  }

  orion::BSONArray condsArr = conds.arr();
  b->append(CSUB_CONDITIONS, condsArr);
  LM_T(LmtMongo, ("Subscription conditions: %s", condsArr.toString().c_str()));
}



/* ****************************************************************************
*
* setLastNotification -
*/
void setLastNotification(long long lastNotification, orion::BSONObjBuilder* b)
{
  b->append(CSUB_LASTNOTIFICATION, lastNotification);
  LM_T(LmtMongo, ("Subscription lastNotification: %lu", lastNotification));
}



/* ****************************************************************************
*
* setCount -
*/
void setCount(long long count, orion::BSONObjBuilder* b)
{
  b->append(CSUB_COUNT, count);
  LM_T(LmtMongo, ("Subscription count: %lu", count));
}



/* ****************************************************************************
*
* setLastFailure -
*/
void setLastFailure(long long lastFailure, const std::string& lastFailureReason, orion::BSONObjBuilder* b)
{
  b->append(CSUB_LASTFAILURE, lastFailure);
  LM_T(LmtMongo, ("Subscription lastFailure: %lu", lastFailure));

  if (!lastFailureReason.empty())
  {
    b->append(CSUB_LASTFAILUREASON, lastFailureReason);
    LM_T(LmtMongo, ("Subscription lastFailureReason: %s", lastFailureReason.c_str()));
  }
}



/* ****************************************************************************
*
* setLastSuccess -
*/
void setLastSuccess(long long lastSuccess, long long lastSuccessCode, orion::BSONObjBuilder* b)
{
  b->append(CSUB_LASTSUCCESS, lastSuccess);
  LM_T(LmtMongo, ("Subscription lastSuccess: %lu", lastSuccess));

  if (lastSuccessCode != -1)
  {
    b->append(CSUB_LASTSUCCESSCODE, lastSuccessCode);
    LM_T(LmtMongo, ("Subscription lastSuccessCode: %lu", lastSuccessCode));
  }
}



/* ****************************************************************************
*
* setExpression -
*/
void setExpression(const Subscription& sub, orion::BSONObjBuilder* b)
{
  orion::BSONObjBuilder expressionB;

  // FIXME #3774: previously this part was based in streamming instead of append()
  expressionB.append(CSUB_EXPR_Q, sub.subject.condition.expression.q);
  expressionB.append(CSUB_EXPR_MQ, sub.subject.condition.expression.mq);
  expressionB.append(CSUB_EXPR_GEOM, sub.subject.condition.expression.geometry);
  expressionB.append(CSUB_EXPR_COORDS, sub.subject.condition.expression.coords);
  expressionB.append(CSUB_EXPR_GEOREL,  sub.subject.condition.expression.georel);

  orion::BSONObj expression = expressionB.obj();

  b->append(CSUB_EXPR, expression);
  LM_T(LmtMongo, ("Subscription expression: %s", expression.toString().c_str()));
}



/* ****************************************************************************
*
* setFormat -
*/
void setFormat(const Subscription& sub, orion::BSONObjBuilder* b)
{
  std::string format = renderFormatToString(sub.attrsFormat);

  b->append(CSUB_FORMAT, format);
  LM_T(LmtMongo, ("Subscription format: %s", format.c_str()));
}



/* ****************************************************************************
*
* setBlacklist -
*/
void setBlacklist(const Subscription& sub, orion::BSONObjBuilder* b)
{
  bool bl = sub.notification.blacklist;

  b->append(CSUB_BLACKLIST, bl);
  LM_T(LmtMongo, ("Subscription blacklist: %s", bl ? "true" : "false"));
}



/* ****************************************************************************
*
* setOnlyChanged -
*/
void setOnlyChanged(const Subscription& sub, orion::BSONObjBuilder* b)
{
  bool bl = sub.notification.onlyChanged;

  b->append(CSUB_ONLYCHANGED, bl);
  LM_T(LmtMongo, ("Subscription onlyChanged: %s", bl ? "true" : "false"));
}


/* ****************************************************************************
*
* setCovered -
*/
void setCovered(const Subscription& sub, orion::BSONObjBuilder* b)
{
  bool bl = sub.notification.covered;

  b->append(CSUB_COVERED, bl);
  LM_T(LmtMongo, ("Subscription covered: %s", bl ? "true" : "false"));
}


/* ****************************************************************************
*
* setNotifyOnMetadataChange -
*/
void setNotifyOnMetadataChange(const Subscription& sub, orion::BSONObjBuilder* b)
{
  bool bl = sub.subject.condition.notifyOnMetadataChange;
  b->append(CSUB_NOTIFYONMETADATACHANGE, bl);
  LM_T(LmtMongo, ("Subscription notifyOnMetadataChange: %s", bl ? "true" : "false"));
}


/* ****************************************************************************
*
* setOperations -
*/
void setOperations(const Subscription& sub, orion::BSONObjBuilder* b)
{
  orion::BSONArrayBuilder operations;

  for (unsigned int ix = 0; ix < sub.subject.condition.altTypes.size(); ++ix)
  {
    operations.append(subAltType2string(sub.subject.condition.altTypes[ix]));
  }

  orion::BSONArray operationsArr = operations.arr();

  b->append(CSUB_ALTTYPES, operationsArr);
  LM_T(LmtMongo, ("Subscription operations: %s", operationsArr.toString().c_str()));
}



/* ****************************************************************************
*
* setMetadata -
*/
void setMetadata(const Subscription& sub, orion::BSONObjBuilder* b)
{
  orion::BSONArrayBuilder metadata;

  for (unsigned int ix = 0; ix < sub.notification.metadata.size(); ++ix)
  {
    metadata.append(sub.notification.metadata[ix]);
  }

  orion::BSONArray metadataArr = metadata.arr();

  b->append(CSUB_METADATA, metadataArr);
  LM_T(LmtMongo, ("Subscription metadata: %s", metadataArr.toString().c_str()));
}
