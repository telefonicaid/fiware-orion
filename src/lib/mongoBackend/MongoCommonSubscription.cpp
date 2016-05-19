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

#include "mongoBackend/MongoCommonSubscription.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/defaultValues.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/MongoGlobal.h"

using namespace ngsiv2;
using namespace mongo;



/* ****************************************************************************
*
* setNewSubscriptionId -
*
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
*
*/
void setExpiration(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_EXPIRATION, sub.expires);
  LM_T(LmtMongo, ("Subscription expiration: %lu", sub.expires));
}



/* ****************************************************************************
*
* setExtendedHttpInfo -
*
*/
static void setExtendedHttpInfo(const HttpInfo& httpInfo, BSONObjBuilder* b)
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
*
*/
void setHttpInfo(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_REFERENCE, sub.notification.httpInfo.url);
  b->append(CSUB_EXTENDED,  sub.notification.httpInfo.extended);

  LM_T(LmtMongo, ("Subscription reference: %s", sub.notification.httpInfo.url.c_str()));
  LM_T(LmtMongo, ("Subscription extended: %s", sub.notification.httpInfo.extended? "true" : "false"));

  if (sub.notification.httpInfo.extended)
  {
    setExtendedHttpInfo(sub.notification.httpInfo, b);
  }
}



/* ****************************************************************************
*
* setThrottling -
*
*/
void setThrottling(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_THROTTLING, sub.throttling);
  LM_T(LmtMongo, ("Subscription throttling: %lu", sub.throttling));
}



/* ****************************************************************************
*
* setServicePath -
*
*/
void setServicePath(const std::string servicePath, BSONObjBuilder* b)
{
  b->append(CSUB_SERVICE_PATH, servicePath);
  LM_T(LmtMongo, ("Subscription servicePath: %s", servicePath.c_str()));
}



/* ****************************************************************************
*
* setDescription -
*
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
*
*/
void setStatus(const Subscription& sub, BSONObjBuilder* b)
{
  std::string status = sub.status == ""?  STATUS_ACTIVE : sub.status;
  b->append(CSUB_STATUS, status);
  LM_T(LmtMongo, ("Subscription status: %s", status.c_str()));
}



/* ****************************************************************************
*
* setEntities -
*
*/
void setEntities(const Subscription& sub, BSONObjBuilder* b)
{
  BSONArrayBuilder entities;
  for (unsigned int ix = 0; ix < sub.subject.entities.size(); ++ix)
  {
    EntID en = sub.subject.entities[ix];

    if (en.type == "")
    {
      if (en.id != "")
      {
        entities.append(BSON(CSUB_ENTITY_ID << en.id << CSUB_ENTITY_ISPATTERN << "false"));
      }
      else // idPattern
      {
        entities.append(BSON(CSUB_ENTITY_ID << en.idPattern << CSUB_ENTITY_ISPATTERN << "true"));
      }
    }
    else // type != ""
    {
      if (en.id != "")
      {
        entities.append(BSON(CSUB_ENTITY_ID << en.id <<
                             CSUB_ENTITY_TYPE << en.type <<
                             CSUB_ENTITY_ISPATTERN << "false"));
      }
      else // idPattern
      {
        entities.append(BSON(CSUB_ENTITY_ID << en.idPattern <<
                             CSUB_ENTITY_TYPE << en.type <<
                             CSUB_ENTITY_ISPATTERN << "true"));
      }
    }
  }
  BSONArray entitiesArr = entities.arr();
  b->append(CSUB_ENTITIES, entitiesArr);
  LM_T(LmtMongo, ("Subscription entities: %s", entitiesArr.toString().c_str()));
}



/* ****************************************************************************
*
* setAttrs -
*
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
*
*/
void setCondsAndInitialNotify
(
  const Subscription&              sub,
  const std::string&               subId,
  const std::string&               status,
  const std::string&               url,
  RenderFormat                     attrsFormat,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,  
  BSONObjBuilder*                  b,
  bool*                            notificationDone
)
{
  // Note that we cannot use status, url and attrsFormat from sub.status, as sub object
  // could correspond to an update and the fields be missing (in which case the one from
  // the original subscription has to be taken; the caller deal with that)

  /* Conds vector (and maybe an initial notification) */
  *notificationDone = false;
  BSONArray  conds = processConditionVector(sub.subject.condition.attributes,
                                            sub.subject.entities,
                                            sub.notification.attributes,
                                            subId,
                                            url,
                                            notificationDone,
                                            attrsFormat,
                                            tenant,
                                            xauthToken,
                                            servicePathV,
                                            &(sub.restriction),
                                            status,
                                            fiwareCorrelator,
                                            sub.notification.attributes);

  b->append(CSUB_CONDITIONS, conds);
  LM_T(LmtMongo, ("Subscription conditions: %s", conds.toString().c_str()));
}



/* ****************************************************************************
*
* setLastNotification -
*
*/
void setLastNotification(long long lastNotification, BSONObjBuilder* b)
{
  b->append(CSUB_LASTNOTIFICATION, lastNotification);
  LM_T(LmtMongo, ("Subscription lastNotification: %lu", lastNotification));
}



/* ****************************************************************************
*
* setCount -
*
*/
void setCount(long long count, BSONObjBuilder* b)
{
  b->append(CSUB_COUNT, count);
  LM_T(LmtMongo, ("Subscription count: %lu", count));
}



/* ****************************************************************************
*
* setExpression -
*
*/
void setExpression(const Subscription& sub, BSONObjBuilder* b)
{
  BSONObj expression = BSON(CSUB_EXPR_Q      << sub.subject.condition.expression.q        <<
                            CSUB_EXPR_GEOM   << sub.subject.condition.expression.geometry <<
                            CSUB_EXPR_COORDS << sub.subject.condition.expression.coords   <<
                            CSUB_EXPR_GEOREL << sub.subject.condition.expression.georel
                           );
  b->append(CSUB_EXPR, expression);
  LM_T(LmtMongo, ("Subscription expression: %s", expression.toString().c_str()));
}



/* ****************************************************************************
*
* setFormat -
*
*/
void setFormat(const Subscription& sub, BSONObjBuilder* b)
{
  std::string format = renderFormatToString(sub.attrsFormat);
  b->append(CSUB_FORMAT, format);
  LM_T(LmtMongo, ("Subscription format: %s", format.c_str()));
}
