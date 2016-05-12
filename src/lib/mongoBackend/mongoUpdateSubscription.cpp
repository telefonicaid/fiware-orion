/*
*
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
* Author: Fermin Galan
*/

#include <map>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/OrionError.h"
#include "alarmMgr/alarmMgr.h"
#include "apiTypesV2/SubscriptionUpdate.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonSubscription.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"

#include "mongo/client/dbclient.h"

using namespace mongo;
using namespace ngsiv2;

/* ****************************************************************************
*
* setExpiration -
*
*/
static void setExpiration(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.expiresProvided)
  {
    setExpiration(subUp, b);
  }
  else
  {
    if (subOrig.hasField(CSUB_EXPIRATION))
    {
      long long expires = getLongFieldF(subOrig, CSUB_EXPIRATION);
      b->append(CSUB_EXPIRATION, expires);
      LM_T(LmtMongo, ("Subscription expiration: %lu", expires));
    }
  }
}



/* ****************************************************************************
*
* setHttpInfo -
*
*/
static void setHttpInfo(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.notificationProvided)
  {
    setHttpInfo(subUp, b);
  }
  else
  {
    // reference is a mandatory field and extended has a clear mapping to false in the case of missing field
    std::string reference = getStringFieldF(subOrig, CSUB_REFERENCE);
    bool extended         = subOrig.hasField(CSUB_EXTENDED) ? getBoolFieldF(subOrig, CSUB_EXTENDED) : false;

    b->append(CSUB_REFERENCE, reference);
    b->append(CSUB_EXTENDED,  extended);

    LM_T(LmtMongo, ("Subscription reference: %s", reference.c_str()));
    LM_T(LmtMongo, ("Subscription extended: %s", extended? "true" : "false"));

    if (subOrig.hasField(CSUB_METHOD))
    {
      std::string method = getStringFieldF(subOrig, CSUB_METHOD);
      b->append(CSUB_METHOD, method);
      LM_T(LmtMongo, ("Subscription method: %s", method.c_str()));
    }

    if (subOrig.hasField(CSUB_HEADERS))
    {
      BSONObj headers = getObjectFieldF(subOrig, CSUB_HEADERS);
      b->append(CSUB_HEADERS, headers);
      LM_T(LmtMongo, ("Subscription headers: %s", headers.toString().c_str()));
    }

    if (subOrig.hasField(CSUB_QS))
    {
      BSONObj qs = getObjectFieldF(subOrig, CSUB_QS);
      b->append(CSUB_QS, qs);
      LM_T(LmtMongo, ("Subscription qs: %s", qs.toString().c_str()));
    }

    if (subOrig.hasField(CSUB_PAYLOAD))
    {
      std::string payload = getStringFieldF(subOrig, CSUB_PAYLOAD);
      b->append(CSUB_PAYLOAD, payload);
      LM_T(LmtMongo, ("Subscription payload: %s", payload.c_str()));
    }
  }
}



/* ****************************************************************************
*
* setThrottling -
*
*/
static void setThrottling(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.throttlingProvided)
  {
    setThrottling(subUp, b);
  }
  else
  {
    if (subOrig.hasField(CSUB_THROTTLING))
    {
      long long throttling = getLongFieldF(subOrig, CSUB_THROTTLING);
      b->append(CSUB_THROTTLING, throttling);
      LM_T(LmtMongo, ("Subscription throttling: %lu", throttling));
    }
  }
}



/* ****************************************************************************
*
* setDescription -
*
*/
static void setDescription(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.descriptionProvided)
  {
    setDescription(subUp, b);
  }
  else
  {
    if (subOrig.hasField(CSUB_DESCRIPTION))
    {
      std::string description = getStringFieldF(subOrig, CSUB_DESCRIPTION);
      b->append(CSUB_DESCRIPTION, description);
      LM_T(LmtMongo, ("Subscription description: %s", description.c_str()));
    }
  }
}



/* ****************************************************************************
*
* setStatus -
*
*/
static void setStatus(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.statusProvided)
  {
    setStatus(subUp, b);
  }
  else
  {
    if (subOrig.hasField(CSUB_STATUS))
    {
      std::string status = getStringFieldF(subOrig, CSUB_STATUS);
      b->append(CSUB_STATUS, status);
      LM_T(LmtMongo, ("Subscription status: %s", status.c_str()));
    }
  }
}



/* ****************************************************************************
*
* setEntities -
*
*/
static void setEntities(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.subjectProvided)
  {
    setEntities(subUp, b);
  }
  else
  {
    BSONArray entities = getArrayFieldF(subOrig, CSUB_ENTITIES);
    b->append(CSUB_ENTITIES, entities);
    LM_T(LmtMongo, ("Subscription entities: %s", entities.toString().c_str()));
  }
}


/* ****************************************************************************
*
* setAttrs -
*
*/
static void setAttrs(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.subjectProvided)
  {
    setAttrs(subUp, b);
  }
  else
  {
    BSONArray attrs = getArrayFieldF(subOrig, CSUB_ATTRS);
    b->append(CSUB_ATTRS, attrs);
    LM_T(LmtMongo, ("Subscription attrs: %s", attrs.toString().c_str()));
  }
}


/* ****************************************************************************
*
* setCondsAndInitialNotify -
*
*/
static void setCondsAndInitialNotify
(
  const SubscriptionUpdate&        subUp,
  const BSONObj&                   subOrig,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  BSONObjBuilder*                  b
)
{
  if (subUp.subjectProvided)
  {
    setCondsAndInitialNotify(subUp, subUp.id, tenant, servicePathV, xauthToken, fiwareCorrelator, false, b);
  }
  else
  {
    BSONArray conds = getArrayFieldF(subOrig, CSUB_CONDITIONS);
    b->append(CSUB_CONDITIONS, conds);
    LM_T(LmtMongo, ("Subscription conditions: %s", conds.toString().c_str()));

    if (subOrig.hasField(CSUB_LASTNOTIFICATION))
    {
      long long lastNotification = getLongFieldF(subOrig, CSUB_LASTNOTIFICATION);
      b->append(CSUB_LASTNOTIFICATION, lastNotification);
      LM_T(LmtMongo, ("Subscription lastNotification: %lu", lastNotification));
    }

    if (subOrig.hasField(CSUB_COUNT))
    {
      long long count = getLongFieldF(subOrig, CSUB_COUNT);
      b->append(CSUB_COUNT, count);
      LM_T(LmtMongo, ("Subscription lastNotification: %lu", count));
    }
  }
}



/* ****************************************************************************
*
* setExpression -
*
*/
static void setExpression(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.subjectProvided)
  {
    setExpression(subUp, b);
  }
  else
  {
    BSONObj expression = getObjectFieldF(subOrig, CSUB_EXPR);
    b->append(CSUB_EXPR, expression);
    LM_T(LmtMongo, ("Subscription expression: %s", expression.toString().c_str()));
  }
}



/* ****************************************************************************
*
* setFormat -
*
*/
static void setFormat(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.attrsFormatProvided)
  {
    setFormat(subUp, b);
  }
  else
  {
    std::string format = getStringFieldF(subOrig, CSUB_FORMAT);
    b->append(CSUB_FORMAT, format);
    LM_T(LmtMongo, ("Subscription format: %s", format.c_str()));
  }
}



/* ****************************************************************************
*
* mongoUpdateSubscription -
*
* Returns:
* - subId: subscription susscessfully updated ('oe' must be ignored), the subId
*   must be used to fill Location header
* - "": subscription creation fail (look to 'oe')
*/
std::string mongoUpdateSubscription
(
  const SubscriptionUpdate&            subUp,
  OrionError*                          oe,
  std::map<std::string, std::string>&  uriParams,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  const std::string&                   xauthToken,
  const std::string&                   fiwareCorrelator
)
{

  bool reqSemTaken = false;

  reqSemTake(__FUNCTION__, "ngsiv2 update subscription request", SemWriteOp, &reqSemTaken);

  // Get subscription from DB
  SubscriptionId subId(subUp.id);
  StatusCode     sc;
  OID            id;
  BSONObj        subOrig;
  if (!safeGetSubId(subId, &id, &sc))
  {
    reqSemGive(__FUNCTION__, "ngsiv2 update subscription request", reqSemTaken);
    std::string details;
    if (sc.code == SccContextElementNotFound)
    {
      details = std::string("invalid OID mimeType: '") + subUp.id + "'";
      alarmMgr.badInput(clientIp, details);
    }
    else // SccReceiverInternalError
    {
      details = std::string("exception getting OID: ") + sc.details;
      LM_E(("Runtime Error (%s)", details.c_str()));
    }
    oe->fill(sc.code, details);
    return "";
  }

  std::string err;
  if (!collectionFindOne(getSubscribeContextCollectionName(tenant), BSON("_id" << id), &subOrig, &err))
  {
    reqSemGive(__FUNCTION__, "ngsiv2 update subscription request (mongo db exception)", reqSemTaken);
    oe->fill(SccReceiverInternalError, err);
    return "";
  }

  if (subOrig.isEmpty())
  {
    reqSemGive(__FUNCTION__, "ngsiv2 update subscription request (no subscriptions found)", reqSemTaken);
    oe->fill(SccContextElementNotFound, "subscription id not found");
    return "";
  }

  // Build the BSON object (using subOrig as starting point)
  BSONObjBuilder b;

  setExpiration(subUp, subOrig, &b);
  setHttpInfo(subUp, subOrig, &b);
  setThrottling(subUp, subOrig, &b);
  setServicePath(servicePathV, &b);
  setDescription(subUp, subOrig, &b);
  setStatus(subUp, subOrig, &b);
  setEntities(subUp, subOrig, &b);
  setAttrs(subUp, subOrig, &b);
  setCondsAndInitialNotify(subUp, subOrig, tenant, servicePathV, xauthToken, fiwareCorrelator, &b);
  setExpression(subUp, subOrig, &b);
  setFormat(subUp, subOrig, &b);

  BSONObj doc = b.obj();

  // Update in DB
  if (!collectionUpdate(getSubscribeContextCollectionName(tenant), BSON("_id" << OID(subUp.id)), doc, false, &err))
  {
    reqSemGive(__FUNCTION__, "ngsiv2 update subscription request (mongo db exception)", reqSemTaken);
    oe->fill(SccReceiverInternalError, err);
    return "";
  }

  // Update in cache
  // TBD

  reqSemGive(__FUNCTION__, "ngsiv2 update subscription request", reqSemTaken);

  return subUp.id;
}
