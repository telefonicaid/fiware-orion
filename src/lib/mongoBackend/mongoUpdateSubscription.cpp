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

#include "mongoBackend/MongoCommonSubscription.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/defaultValues.h"
#include "common/statistics.h"
#include "apiTypesV2/SubscriptionUpdate.h"
#include "rest/OrionError.h"
#include "alarmMgr/alarmMgr.h"
#include "cache/subCache.h"

#include "mongoBackend/MongoGlobal.h"  // composeDatabaseName
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoSubCache.h"
#include "mongoBackend/mongoUpdateSubscription.h"

#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/safeMongo.h"
#include "mongoDriver/BSONObjBuilder.h"


/* ****************************************************************************
*
* USING
*/
using ngsiv2::HttpInfo;
using ngsiv2::Subscription;
using ngsiv2::SubscriptionUpdate;
using ngsiv2::EntID;


/* ****************************************************************************
*
* setNotificationInfo -
*
* This function does the cleanup ($unset) corresponding to a potential change
* of notification type. Then, it calls the setNotificationInfo() in MongoCommonSubscription.h/cpp
*/
void setNotificationInfo(const Subscription& sub, orion::BSONObjBuilder* setB, orion::BSONObjBuilder* unsetB)
{
  if (sub.notification.type == ngsiv2::HttpNotification)
  {
    unsetB->append(CSUB_MQTTTOPIC, 1);
    unsetB->append(CSUB_MQTTQOS,   1);
    unsetB->append(CSUB_MQTTRETAIN, 1);
    unsetB->append(CSUB_USER,      1);
    unsetB->append(CSUB_PASSWD,    1);

    if  (sub.notification.httpInfo.payloadType == ngsiv2::CustomPayloadType::Text)
    {
      unsetB->append(CSUB_JSON, 1);
      unsetB->append(CSUB_NGSI, 1);
      // Sometimes there is no payload in the sub request, in which case we also have to unset
      if ((sub.notification.httpInfo.includePayload) && (sub.notification.httpInfo.payload.empty()))
      {
        unsetB->append(CSUB_PAYLOAD, 1);
      }
    }
    else if (sub.notification.httpInfo.payloadType == ngsiv2::CustomPayloadType::Json)
    {
      unsetB->append(CSUB_PAYLOAD, 1);
      unsetB->append(CSUB_NGSI, 1);
    }
    else  // (sub.notification.httpInfo.payloadType == ngsiv2::CustomPayloadType::Ngsi)
    {
      unsetB->append(CSUB_PAYLOAD, 1);
      unsetB->append(CSUB_JSON, 1);
    }
  }
  else  // MqttNotification
  {
    unsetB->append(CSUB_TIMEOUT, 1);
    unsetB->append(CSUB_METHOD,  1);
    unsetB->append(CSUB_HEADERS, 1);
    unsetB->append(CSUB_QS,      1);

    if (!sub.notification.mqttInfo.providedAuth)
    {
      unsetB->append(CSUB_USER,   1);
      unsetB->append(CSUB_PASSWD, 1);
    }

    if  (sub.notification.mqttInfo.payloadType == ngsiv2::CustomPayloadType::Text)
    {
      unsetB->append(CSUB_JSON, 1);
      unsetB->append(CSUB_NGSI, 1);
      // Sometimes there is no payload in the sub request, in which case we also have to unset
      if ((sub.notification.mqttInfo.includePayload) && (sub.notification.mqttInfo.payload.empty()))
      {
        unsetB->append(CSUB_PAYLOAD, 1);
      }
    }
    else if (sub.notification.mqttInfo.payloadType == ngsiv2::CustomPayloadType::Json)
    {
      unsetB->append(CSUB_PAYLOAD, 1);
      unsetB->append(CSUB_NGSI, 1);
    }
    else  // (sub.notification.mqttInfo.payloadType == ngsiv2::CustomPayloadType::Ngsi)
    {
      unsetB->append(CSUB_JSON, 1);
      unsetB->append(CSUB_PAYLOAD, 1);
    }
  }

  setNotificationInfo(sub, setB);
}



/* ****************************************************************************
*
* updateInCache -
*/
static void updateInCache
(
  const orion::BSONObj&      doc,
  const SubscriptionUpdate&  subUp,
  const std::string&         tenant
)
{
  //
  // StringFilter in Scope?
  //
  // Any Scope of type SCOPE_TYPE_SIMPLE_QUERY in subUp.restriction.scopeVector?
  // If so, set it as string filter to the sub-cache item
  //
  StringFilter*  stringFilterP   = NULL;
  StringFilter*  mdStringFilterP = NULL;

  for (unsigned int ix = 0; ix < subUp.restriction.scopeVector.size(); ++ix)
  {
    if (subUp.restriction.scopeVector[ix]->type == SCOPE_TYPE_SIMPLE_QUERY)
    {
      stringFilterP = subUp.restriction.scopeVector[ix]->stringFilterP;
    }

    if (subUp.restriction.scopeVector[ix]->type == SCOPE_TYPE_SIMPLE_QUERY_MD)
    {
      mdStringFilterP = subUp.restriction.scopeVector[ix]->mdStringFilterP;
    }
  }

  //
  // Modification of the subscription cache
  //
  // The subscription "before this update" is looked up in cache and referenced by 'cSubP'.
  // The "updated subscription information" is in 'newSubObject' (mongo BSON object format).
  //
  // All we need to do now for the cache is to:
  //   1. Remove 'cSubP' from sub-cache (if present)
  //   2. Create 'newSubObject' in sub-cache (if applicable)
  //
  // The subscription is already updated in mongo.
  //
  //
  // There are four different scenarios here:
  //   1. Old sub was in cache, new sub enters cache
  //   2. Old sub was NOT in cache, new sub enters cache
  //   3. Old subwas in cache, new sub DOES NOT enter cache
  //   4. Old sub was NOT in cache, new sub DOES NOT enter cache
  //
  // This is resolved by two separate functions, one that removes the old one,
  // if found (subCacheItemLookup+subCacheItemRemove), and the other one that inserts the sub,
  // IF it should be inserted (subCacheItemInsert).
  // If inserted, subCacheUpdateStatisticsIncrement is called to update the statistics counter of insertions.
  //

  std::string         q;
  std::string         mq;
  std::string         geom;
  std::string         coords;
  std::string         georel;
  RenderFormat        renderFormat = NGSI_V2_NORMALIZED;  // Default value

  // 0. Field extraction from doc is done before cacheSemTake() to save some small time
  if (doc.hasField(CSUB_FORMAT))
  {
    renderFormat = stringToRenderFormat(getStringFieldF(doc, CSUB_FORMAT));
  }

  if (doc.hasField(CSUB_EXPR))
  {
    orion::BSONObj expr = getObjectFieldF(doc, CSUB_EXPR);

    q      = expr.hasField(CSUB_EXPR_Q)?      getStringFieldF(expr, CSUB_EXPR_Q)      : "";
    mq     = expr.hasField(CSUB_EXPR_MQ)?     getStringFieldF(expr, CSUB_EXPR_MQ)     : "";
    geom   = expr.hasField(CSUB_EXPR_GEOM)?   getStringFieldF(expr, CSUB_EXPR_GEOM)   : "";
    coords = expr.hasField(CSUB_EXPR_COORDS)? getStringFieldF(expr, CSUB_EXPR_COORDS) : "";
    georel = expr.hasField(CSUB_EXPR_GEOREL)? getStringFieldF(expr, CSUB_EXPR_GEOREL) : "";
  }

  // 1. Lookup matching subscription in subscription-cache

  cacheSemTake(__FUNCTION__, "Updating cached subscription");

  CachedSubscription* subCacheP        = subCacheItemLookup(tenant.c_str(), subUp.id.c_str());
  char*               servicePathCache = (char*) ((subCacheP == NULL)? "" : subCacheP->servicePath);

  LM_T(LmtSubCache, ("update: %s", doc.toString().c_str()));

  long long    lastNotificationTime;
  long long    lastFailure;
  std::string  lastFailureReason;
  long long    lastSuccess;
  long long    lastSuccessCode;
  long long    count;
  long long    failsCounter;
  long long    failsCounterFromDb;
  bool         failsCounterFromDbValid;
  std::string  status;
  double       statusLastChange;

  if (subCacheP != NULL)
  {
    lastNotificationTime = subCacheP->lastNotificationTime;
    lastFailure          = subCacheP->lastFailure;
    lastFailureReason    = subCacheP->lastFailureReason;
    lastSuccess          = subCacheP->lastSuccess;
    lastSuccessCode      = subCacheP->lastSuccessCode;
    count                = subCacheP->count;
    failsCounter         = subCacheP->failsCounter;
    failsCounterFromDb   = subCacheP->failsCounterFromDb;
    failsCounterFromDbValid = subCacheP->failsCounterFromDbValid;
    status               = subCacheP->status;
    statusLastChange     = subCacheP->statusLastChange;
  }
  else
  {
    lastNotificationTime = -1;
    lastFailure          = -1;
    lastFailureReason    = "";
    lastSuccess          = -1;
    lastSuccessCode      = -1;
    count                = 0;
    failsCounter         = 0;
    failsCounterFromDb   = 0;
    failsCounterFromDbValid = false;
    status               = "";
    statusLastChange     = -1;
  }

  // different for other fields grabbed from the cache, status could be included in the sub update
  // thus, effective status is the newest one in cache or in DB
  double statusLastChangeAtDb = doc.hasField(CSUB_STATUS_LAST_CHANGE)? getNumberFieldF(doc, CSUB_STATUS_LAST_CHANGE) : -1;
  std::string statusAtDb      = doc.hasField(CSUB_STATUS)?             getStringFieldF(doc, CSUB_STATUS)             : "";

  std::string effectiveStatus      = statusLastChange > statusLastChangeAtDb ? status           : statusAtDb;
  double effectiveStatusLastChante = statusLastChange > statusLastChangeAtDb ? statusLastChange : statusLastChangeAtDb;

  int mscInsert = mongoSubCacheItemInsert(tenant.c_str(),
                                          doc,
                                          subUp.id.c_str(),
                                          servicePathCache,
                                          lastNotificationTime,
                                          lastFailure,
                                          lastFailureReason,
                                          lastSuccess,
                                          lastSuccessCode,
                                          count,
                                          failsCounter,
                                          failsCounterFromDb,
                                          failsCounterFromDbValid,
                                          doc.hasField(CSUB_EXPIRATION)? getLongFieldF(doc, CSUB_EXPIRATION) : 0,
                                          effectiveStatus,
                                          effectiveStatusLastChante,
                                          q,
                                          mq,
                                          geom,
                                          coords,
                                          georel,
                                          stringFilterP,
                                          mdStringFilterP,
                                          renderFormat);

  if (mscInsert == 0)  // 0: Insertion was really made
  {
    subCacheUpdateStatisticsIncrement();

    if (subCacheP != NULL)
    {
      LM_T(LmtSubCache, ("Calling subCacheItemRemove"));
      subCacheItemRemove(subCacheP);
    }
  }

  cacheSemGive(__FUNCTION__, "Updating cached subscription");
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
  const SubscriptionUpdate&        subUp,
  OrionError*                      oe,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV
)
{
  bool reqSemTaken = false;

  reqSemTake(__FUNCTION__, "ngsiv2 update subscription request", SemWriteOp, &reqSemTaken);

  std::string      servicePath = servicePathV[0].empty() ? SERVICE_PATH_ALL : servicePathV[0];
  double           now         = getCurrentTime();

  // Previous versions of the sub up logic calculate the final document mixing the
  // existing one in DB plus the additions from cache. However, this is complex and involve
  // more operations on MongoDB that the current approach, based in findAndModify and
  // in $set and $unset operations
  //
  // Note also that "dynamic fields" that cannot be updated with PATCH subscription
  // (count, lastNotification, etc.) are not touched: they have their own
  // updating logic in other places of the code (in cache sync logic)

  orion::BSONObjBuilder setB;
  orion::BSONObjBuilder unsetB;

  setServicePath(servicePath, &setB);

  if (subUp.subjectProvided)       setEntities(subUp, &setB, subUp.fromNgsiv1);
  if (subUp.subjectProvided)       setConds(subUp, &setB);
  if (subUp.subjectProvided)       setOperations(subUp, &setB);
  if (subUp.subjectProvided)       setExpression(subUp, &setB);
  if (subUp.notificationProvided)  setNotificationInfo(subUp, &setB, &unsetB);
  if (subUp.notificationProvided)  setAttrs(subUp, &setB);
  if (subUp.notificationProvided)  setMetadata(subUp, &setB);
  if (subUp.notificationProvided)  setMaxFailsLimit(subUp, &setB);
  if (subUp.expiresProvided)       setExpiration(subUp, &setB);
  if (subUp.throttlingProvided)    setThrottling(subUp, &setB);
  if (subUp.statusProvided)        setStatus(subUp.status, &setB, now);
  if (subUp.blacklistProvided)     setBlacklist(subUp, &setB);
  if (subUp.onlyChangedProvided)   setOnlyChanged(subUp, &setB);
  if (subUp.coveredProvided)       setCovered(subUp, &setB);
  if (subUp.notifyOnMetadataChangeProvided) setNotifyOnMetadataChange(subUp, &setB);
  if (subUp.attrsFormatProvided)   setFormat(subUp, &setB);

  // Description is special, as "" value removes the field
  if (subUp.descriptionProvided)
  {
    if (subUp.description.empty())
    {
      setDescription(subUp, &unsetB);
    }
    else
    {
      setDescription(subUp, &setB);
    }
  }

  orion::BSONObjBuilder update;
  if (setB.nFields() > 0)
  {
    update.append("$set", setB.obj());
  }
  if (unsetB.nFields() > 0)
  {
    update.append("$unset", unsetB.obj());
  }

  // Update in DB
  orion::BSONObjBuilder id;
  id.append("_id", orion::OID(subUp.id));

  std::string err;
  orion::BSONObj result;
  if (!collectionFindAndModify(composeDatabaseName(tenant), COL_CSUBS, id.obj(), update.obj(), true, &result, &err))
  {
    reqSemGive(__FUNCTION__, "ngsiv2 update subscription request (mongo db exception)", reqSemTaken);
    oe->fill(SccReceiverInternalError, err);

    return "";
  }

  // Note the actual value is not in reply itself, but in the key "value", see
  // https://jira.mongodb.org/browse/CDRIVER-4173
  // If value is null then it means that findAndModify didn't find any matching document
  if (getFieldF(result, "value").isNull())
  {
    reqSemGive(__FUNCTION__, "ngsiv2 update subscription request (no subscriptions found)", reqSemTaken);
    oe->fill(SccContextElementNotFound, "subscription id not found");

    return "";
  }

  // Update in cache
  if (!noCache)
  {
    updateInCache(getObjectFieldF(result, "value"), subUp, tenant);
  }

  reqSemGive(__FUNCTION__, "ngsiv2 update subscription request", reqSemTaken);

  return subUp.id;
}
