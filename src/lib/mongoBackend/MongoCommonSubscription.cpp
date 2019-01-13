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

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/defaultValues.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonSubscription.h"



/* ****************************************************************************
*
* USING - 
*/
using mongo::BSONObjBuilder;
using mongo::BSONArrayBuilder;
using mongo::BSONObj;
using mongo::BSONArray;
using mongo::OID;
using ngsiv2::Subscription;
using ngsiv2::HttpInfo;
using ngsiv2::EntID;



/* ****************************************************************************
*
* setNewSubscriptionId -
*/
std::string setNewSubscriptionId(BSONObjBuilder* b)
{
  OID  oid;

  oid.init();
  b->append("_id", oid);

  LM_T(LmtMongo, ("Subscription _id: %s", oid.toString().c_str()));
  return oid.toString();
}



/* ****************************************************************************
*
* setExpiration -
*/
void setExpiration(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_EXPIRATION, sub.expires);
  LM_T(LmtMongo, ("Subscription expiration: %lu", sub.expires));
}



/* ****************************************************************************
*
* setCustomHttpInfo -
*/
static void setCustomHttpInfo(const HttpInfo& httpInfo, BSONObjBuilder* b)
{
  if (httpInfo.verb != NOVERB)
  {
    std::string method = verbName(httpInfo.verb);

    b->append(CSUB_METHOD, method);
    LM_T(LmtMongo, ("Subscription method: %s", method.c_str()));
  }

  if (httpInfo.headers.size() > 0)
  {
    BSONObjBuilder headersBuilder;

    for (std::map<std::string, std::string>::const_iterator it = httpInfo.headers.begin(); it != httpInfo.headers.end(); ++it)
    {
      headersBuilder.append(it->first, it->second);
    }
    BSONObj headersObj = headersBuilder.obj();

    b->append(CSUB_HEADERS, headersObj);
    LM_T(LmtMongo, ("Subscription headers: %s", headersObj.toString().c_str()));
  }

  if (httpInfo.qs.size() > 0)
  {
    BSONObjBuilder qsBuilder;

    for (std::map<std::string, std::string>::const_iterator it = httpInfo.qs.begin(); it != httpInfo.qs.end(); ++it)
    {
      qsBuilder.append(it->first, it->second);
    }

    BSONObj qsObj = qsBuilder.obj();

    b->append(CSUB_QS, qsObj);
    LM_T(LmtMongo, ("Subscription qs: %s", qsObj.toString().c_str()));
  }

  if (httpInfo.payload != "")
  {
    b->append(CSUB_PAYLOAD, httpInfo.payload);
    LM_T(LmtMongo, ("Subscription payload: %s", httpInfo.payload.c_str()));
  }
}



/* ****************************************************************************
*
* setHttpInfo -
*/
void setHttpInfo(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_REFERENCE, sub.notification.httpInfo.url);
  b->append(CSUB_CUSTOM,    sub.notification.httpInfo.custom);

  LM_T(LmtMongo, ("Subscription reference: %s", sub.notification.httpInfo.url.c_str()));
  LM_T(LmtMongo, ("Subscription custom:    %s", sub.notification.httpInfo.custom? "true" : "false"));

  if (sub.notification.httpInfo.custom)
  {
    setCustomHttpInfo(sub.notification.httpInfo, b);
  }
}



/* ****************************************************************************
*
* setThrottling -
*/
void setThrottling(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_THROTTLING, sub.throttling);
  LM_T(LmtMongo, ("Subscription throttling: %lu", sub.throttling));
}



/* ****************************************************************************
*
* setServicePath -
*/
void setServicePath(const std::string& servicePath, BSONObjBuilder* b)
{
  b->append(CSUB_SERVICE_PATH, servicePath);
  LM_T(LmtMongo, ("Subscription servicePath: %s", servicePath.c_str()));
}



/* ****************************************************************************
*
* setDescription -
*/
void setDescription(const Subscription& sub, BSONObjBuilder* b)
{
  if (sub.description != "")
  {
    b->append(CSUB_DESCRIPTION, sub.description);
    LM_T(LmtMongo, ("Subscription description: %s", sub.description.c_str()));
  }
}



/* ****************************************************************************
*
* setStatus -
*/
void setStatus(const Subscription& sub, BSONObjBuilder* b)
{
  std::string  status = (sub.status == "")? STATUS_ACTIVE : sub.status;

  b->append(CSUB_STATUS, status);
  LM_T(LmtMongo, ("Subscription status: %s", status.c_str()));
}



/* ****************************************************************************
*
* setEntities -
*/
void setEntities(const Subscription& sub, BSONObjBuilder* b)
{
  BSONArrayBuilder entities;

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
    if (en.idPattern != "")
    {
      finalId     = en.idPattern;
      isIdPattern = "true";
    }
    else if (en.id != "")
    {
      finalId     = en.id;
      isIdPattern = "false";
    }

    if (en.typePattern != "")
    {
      finalType     = en.typePattern;
      isTypePattern = true;
    }
    else if (en.type != "")
    {
      finalType     = en.type;
      isTypePattern = false;
    }

    if (finalType.empty())  // no type provided
    {
      entities.append(BSON(CSUB_ENTITY_ID << finalId << CSUB_ENTITY_ISPATTERN << isIdPattern));
    }
    else  // type provided
    {
      entities.append(BSON(CSUB_ENTITY_ID   << finalId   << CSUB_ENTITY_ISPATTERN     << isIdPattern
                        << CSUB_ENTITY_TYPE << finalType << CSUB_ENTITY_ISTYPEPATTERN << isTypePattern));
    }
  }

  BSONArray entitiesArr = entities.arr();

  b->append(CSUB_ENTITIES, entitiesArr);
  LM_T(LmtMongo, ("Subscription entities: %s", entitiesArr.toString().c_str()));
}



/* ****************************************************************************
*
* setAttrs -
*/
void setAttrs(const Subscription& sub, BSONObjBuilder* b)
{
  BSONArrayBuilder attrs;

  for (unsigned int ix = 0; ix < sub.notification.attributes.size(); ++ix)
  {
    attrs.append(sub.notification.attributes[ix]);
  }

  BSONArray attrsArr = attrs.arr();
  b->append(CSUB_ATTRS, attrsArr);
  LM_T(LmtMongo, ("Subscription attributes: %s", attrsArr.toString().c_str()));
}



/* ****************************************************************************
*
* setCondsAndInitialNotify -
*/
void setCondsAndInitialNotify
(
  const Subscription&              sub,
  const std::string&               subId,
  const std::string&               status,
  const std::vector<std::string>&  notifAttributesV,
  const std::vector<std::string>&  metadataV,
  const HttpInfo&                  httpInfo,
  bool                             blacklist,
  RenderFormat                     attrsFormat,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  BSONObjBuilder*                  b,
  bool*                            notificationDone,
  ApiVersion                       apiVersion,
  const bool&                      skipInitialNotification
)
{
  //
  // Note that we cannot use status, url and attrsFormat from sub.status, as sub object
  // could correspond to an update and the fields be missing (in which case the one from
  // the original subscription has to be taken; the caller deal with that)
  //

  /* Conds vector (and maybe an initial notification) */
  *notificationDone = false;

  BSONArray  conds = processConditionVector(sub.subject.condition.attributes,
                                            sub.subject.entities,
                                            notifAttributesV,
                                            metadataV,
                                            subId,
                                            httpInfo,
                                            notificationDone,
                                            attrsFormat,
                                            tenant,
                                            xauthToken,
                                            servicePathV,
                                            &(sub.restriction),
                                            status,
                                            fiwareCorrelator,
                                            notifAttributesV,
                                            blacklist,
                                            apiVersion,
                                            skipInitialNotification);

  b->append(CSUB_CONDITIONS, conds);
  LM_T(LmtMongo, ("Subscription conditions: %s", conds.toString().c_str()));
}



/* ****************************************************************************
*
* setLastNotification -
*/
void setLastNotification(long long lastNotification, BSONObjBuilder* b)
{
  b->append(CSUB_LASTNOTIFICATION, lastNotification);
  LM_T(LmtMongo, ("Subscription lastNotification: %lu", lastNotification));
}



/* ****************************************************************************
*
* setCount -
*/
void setCount(long long count, BSONObjBuilder* b)
{
  b->append(CSUB_COUNT, count);
  LM_T(LmtMongo, ("Subscription count: %lu", count));
}



/* ****************************************************************************
*
* setLastFailure -
*/
void setLastFailure(long long lastFailure, BSONObjBuilder* b)
{
  b->append(CSUB_LASTFAILURE, lastFailure);
  LM_T(LmtMongo, ("Subscription lastFailure: %lu", lastFailure));
}



/* ****************************************************************************
*
* setLastSuccess -
*/
void setLastSuccess(long long lastSuccess, BSONObjBuilder* b)
{
  b->append(CSUB_LASTSUCCESS, lastSuccess);
  LM_T(LmtMongo, ("Subscription lastSuccess: %lu", lastSuccess));
}



/* ****************************************************************************
*
* setExpression -
*/
void setExpression(const Subscription& sub, BSONObjBuilder* b)
{
  BSONObj expression = BSON(CSUB_EXPR_Q      << sub.subject.condition.expression.q        <<
                            CSUB_EXPR_MQ     << sub.subject.condition.expression.mq       <<
                            CSUB_EXPR_GEOM   << sub.subject.condition.expression.geometry <<
                            CSUB_EXPR_COORDS << sub.subject.condition.expression.coords   <<
                            CSUB_EXPR_GEOREL << sub.subject.condition.expression.georel);

  b->append(CSUB_EXPR, expression);
  LM_T(LmtMongo, ("Subscription expression: %s", expression.toString().c_str()));
}



/* ****************************************************************************
*
* setFormat -
*/
void setFormat(const Subscription& sub, BSONObjBuilder* b)
{
  std::string format = renderFormatToString(sub.attrsFormat);

  b->append(CSUB_FORMAT, format);
  LM_T(LmtMongo, ("Subscription format: %s", format.c_str()));
}



/* ****************************************************************************
*
* setBlacklist -
*/
void setBlacklist(const Subscription& sub, BSONObjBuilder* b)
{
  bool bl = sub.notification.blacklist;

  b->append(CSUB_BLACKLIST, bl);
  LM_T(LmtMongo, ("Subscription blacklist: %s", bl ? "true" : "false"));
}



/* ****************************************************************************
*
* setMetadata -
*/
void setMetadata(const Subscription& sub, BSONObjBuilder* b)
{
  BSONArrayBuilder metadata;

  for (unsigned int ix = 0; ix < sub.notification.metadata.size(); ++ix)
  {
    metadata.append(sub.notification.metadata[ix]);
  }

  BSONArray metadataArr = metadata.arr();

  b->append(CSUB_METADATA, metadataArr);
  LM_T(LmtMongo, ("Subscription metadata: %s", metadataArr.toString().c_str()));
}
