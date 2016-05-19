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
#include "mongoBackend/mongoSubCache.h"
#include "common/defaultValues.h"

#include "cache/subCache.h"

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
      long long expires = getIntOrLongFieldAsLongF(subOrig, CSUB_EXPIRATION);
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
      long long throttling = getIntOrLongFieldAsLongF(subOrig, CSUB_THROTTLING);
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
  if (subUp.subjectProvided && !subUp.fromNgsiv1)
  {
    // NGSIv1 doesn't allow to change entities,
    // see https://fiware-orion.readthedocs.io/en/develop/user/updating_regs_and_subs/index.html
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
  if (subUp.notificationProvided)
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
* setCondsAndInitialNotifyNgsiv1 -
*
* This method could be removed along with the rest of NGSIv1 stuff
*
*/
static void setCondsAndInitialNotifyNgsiv1
(
  const Subscription&              sub,
  const BSONObj&                   subOrig,
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
  // Similar to setCondsAndInitialNotify() in MongoCommonSubscripion.cpp, but
  // entities and notifications attributes are not taken from sub but from subOrig
  //
  // Most of the following code is copied from mongoGetSubscription logic but, given
  // that this function is temporal, I don't worry too much about DRY-ness here

  std::vector<EntID>       entities;
  std::vector<BSONElement> ents = getFieldF(subOrig, CSUB_ENTITIES).Array();
  for (unsigned int ix = 0; ix < ents.size(); ++ix)
  {
    BSONObj ent           = ents[ix].embeddedObject();
    std::string id        = getStringFieldF(ent, CSUB_ENTITY_ID);
    std::string type      = ent.hasField(CSUB_ENTITY_TYPE)? getStringFieldF(ent, CSUB_ENTITY_TYPE) : "";
    std::string isPattern = getStringFieldF(ent, CSUB_ENTITY_ISPATTERN);

    EntID en;
    if (isFalse(isPattern))
    {
      en.id = id;
    }
    else
    {
      en.idPattern = id;
    }
    en.type = type;

    entities.push_back(en);
  }

  std::vector<std::string> attributes;
  std::vector<BSONElement> attrs = getFieldF(subOrig, CSUB_ATTRS).Array();
  for (unsigned int ix = 0; ix < attrs.size(); ++ix)
  {
    attributes.push_back(attrs[ix].String());
  }


  /* Conds vector (and maybe and initial notification) */
  *notificationDone = false;
  BSONArray  conds = processConditionVector(sub.subject.condition.attributes,
                                            entities,
                                            attributes,
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
                                            sub.notification.attributes,
                                            sub.notification.blacklist);

  b->append(CSUB_CONDITIONS, conds);
  LM_T(LmtMongo, ("Subscription conditions: %s", conds.toString().c_str()));
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
  BSONObjBuilder*                  b,
  bool*                            notificationDone
)
{
  // notification can be changed to true by setCondsAndInitialNotify()
  *notificationDone = false;

  if (subUp.subjectProvided)
  {
    std::string status;
    if (subUp.statusProvided)
    {
      status = subUp.status;
    }
    else
    {
      status = subOrig.hasField(CSUB_STATUS)? getStringFieldF(subOrig, CSUB_STATUS) : STATUS_ACTIVE;
    }

    std::string url;
    if (subUp.notificationProvided)
    {
      url = subUp.notification.httpInfo.url;
    }
    else
    {
      url = getStringFieldF(subOrig, CSUB_REFERENCE);
    }

    RenderFormat attrsFormat;
    if (subUp.attrsFormatProvided)
    {
      attrsFormat = subUp.attrsFormat;
    }
    else
    {
      attrsFormat = subOrig.hasField(CSUB_FORMAT)? stringToRenderFormat(getStringFieldF(subOrig, CSUB_FORMAT)) : NGSI_V2_NORMALIZED;
    }

    if (subUp.fromNgsiv1)
    {
      // In NGSIv1 is legal updating conditions without updating entities, which is not possible
      // in NGSIv2 (as both entities and coditions are part of 'subject' and they are updated as
      // a whole). In addition, NGSIv1 doesn't allow to update notification attributes. Both
      // (entities and notification attributes) are pased in subOrig
      //
      // See: https://fiware-orion.readthedocs.io/en/develop/user/updating_regs_and_subs/index.html
      setCondsAndInitialNotifyNgsiv1(subUp, subOrig, subUp.id, status, url, attrsFormat,
                                     tenant, servicePathV, xauthToken, fiwareCorrelator,
                                     b, notificationDone);
    }
    else
    {
      setCondsAndInitialNotify(subUp, subUp.id, status, url, attrsFormat,
                               tenant, servicePathV, xauthToken, fiwareCorrelator,
                               b, notificationDone);
    }
  }
  else
  {    
    BSONArray conds = getArrayFieldF(subOrig, CSUB_CONDITIONS);
    b->append(CSUB_CONDITIONS, conds);
    LM_T(LmtMongo, ("Subscription conditions: %s", conds.toString().c_str()));
  }
}



/* ****************************************************************************
*
* setCount -
*
*/
static void setCount(long long inc, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subOrig.hasField(CSUB_COUNT))
  {
    long long count = getIntOrLongFieldAsLongF(subOrig, CSUB_COUNT);
    setCount(count + inc, b);
  }
  else
  {
    // In this case we only add if inc is different from 0
    if (inc > 0)
    {
      setCount(inc, b);
    }
  }
}



/* ****************************************************************************
*
* setLastNotification -
*
*/
static void setLastNotification(const BSONObj& subOrig, CachedSubscription* subCacheP, BSONObjBuilder* b)
{
  // FIXME P1: if CSUB_LASTNOTIFICATION is not in the original doc, it will also not be in the new doc.
  //           Is this necessary?
  //           The implementation would get a lot simpler if we ALWAYS add CSUB_LASTNOTIFICATION and CSUB_COUNT
  //           to 'newSub'
  if (!subOrig.hasField(CSUB_LASTNOTIFICATION))
  {
    return;
  }

  long long lastNotification = getIntOrLongFieldAsLongF(subOrig, CSUB_LASTNOTIFICATION);

  //
  // Compare with 'lastNotificationTime', that might come from the sub-cache.
  // If the cached value of lastNotificationTime is higher, then use it.
  //
  if (subCacheP != NULL && (subCacheP->lastNotificationTime > lastNotification))
  {
    lastNotification = subCacheP->lastNotificationTime;
  }

  setLastNotification(lastNotification, b);
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
* setBlacklist -
*
*/
static void setBlacklist(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.blacklistProvided)
  {
    setBlacklist(subUp, b);
  }
  else
  {
    bool bl = getBoolFieldF(subOrig, CSUB_BLACKLIST);
    b->append(CSUB_BLACKLIST, bl);
    LM_T(LmtMongo, ("Subscription blacklist: %s", bl? "TRUE" : "FALSE"));
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

  // Build the BSON object (using subOrig as starting point plus some info from cache)
  BSONObjBuilder b;
  std::string         servicePath  = servicePathV[0] == "" ? DEFAULT_SERVICE_PATH_QUERIES : servicePathV[0];
  CachedSubscription* subCacheP    = subCacheItemLookup(tenant.c_str(), subUp.id.c_str());
  bool                notificationDone = false;
  long long           lastNotification = 0;

  setExpiration(subUp, subOrig, &b);
  setHttpInfo(subUp, subOrig, &b);
  setThrottling(subUp, subOrig, &b);
  setServicePath(servicePath, &b);
  setDescription(subUp, subOrig, &b);
  setStatus(subUp, subOrig, &b);
  setEntities(subUp, subOrig, &b);
  setAttrs(subUp, subOrig, &b);
  setBlacklist(subUp, subOrig, &b);
  setCondsAndInitialNotify(subUp, subOrig, tenant, servicePathV, xauthToken, fiwareCorrelator,
                           &b, &notificationDone);

  if (notificationDone)
  {
    lastNotification = (long long) getCurrentTime();
    long long countInc         = 1;

    // Update sub-cache
    // FIXME #2146: this is safe without sem?
    //
    if (subCacheP != NULL)
    {
      subCacheP->count                 += 1; // 'count' to be reset later if DB operation OK
      subCacheP->lastNotificationTime  = lastNotification;

      countInc = subCacheP->count;     // already inc with +1
    }

    setLastNotification(lastNotification, &b);
    setCount(countInc, subOrig, &b);
  }
  else
  {
    setLastNotification(subOrig, subCacheP, &b);
    setCount(0, subOrig, &b);
  }

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

  //
  // StringFilter in Scope?
  //
  // Any Scope of type SCOPE_TYPE_SIMPLE_QUERY in subUp.restriction.scopeVector?
  // If so, set it as string filter to the sub-cache item
  //
  StringFilter*  stringFilterP = NULL;

  for (unsigned int ix = 0; ix < subUp.restriction.scopeVector.size(); ++ix)
  {
    if (subUp.restriction.scopeVector[ix]->type == SCOPE_TYPE_SIMPLE_QUERY)
    {
      stringFilterP = subUp.restriction.scopeVector[ix]->stringFilterP;
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
  // This is resolved by two separate functions, one that removes the old one, if found (subCacheItemLookup+subCacheItemRemove),
  // and the other one that inserts the sub, IF it should be inserted (subCacheItemInsert).
  // If inserted, subCacheUpdateStatisticsIncrement is called to update the statistics counter of insertions.
  //


  // 0. Lookup matching subscription in subscription-cache

  cacheSemTake(__FUNCTION__, "Updating cached subscription");

  // Second lookup for the same in the same function. However, we have to do it, as the item in the cache could have been changed
  // in the meanwhile.
  subCacheP = subCacheItemLookup(tenant.c_str(), subUp.id.c_str());

  char* servicePathCache = (char*) ((subCacheP == NULL)? "" : subCacheP->servicePath);

  LM_T(LmtSubCache, ("update: %s", doc.toString().c_str()));

  int mscInsert = mongoSubCacheItemInsert(tenant.c_str(),
                                          doc,
                                          subUp.id.c_str(),
                                          servicePathCache,
                                          lastNotification,
                                          doc.hasField(CSUB_EXPIRATION)? getLongFieldF(doc, CSUB_EXPIRATION) : 0,
                                          doc.hasField(CSUB_STATUS)? getStringFieldF(doc, CSUB_STATUS) : STATUS_ACTIVE,
                                          doc.hasField(CSUB_EXPR)? getStringFieldF(getObjectFieldF(doc, CSUB_EXPR), CSUB_EXPR_Q) : "",
                                          doc.hasField(CSUB_EXPR)? getStringFieldF(getObjectFieldF(doc, CSUB_EXPR), CSUB_EXPR_GEOM) : "",
                                          doc.hasField(CSUB_EXPR)? getStringFieldF(getObjectFieldF(doc, CSUB_EXPR), CSUB_EXPR_COORDS) : "",
                                          doc.hasField(CSUB_EXPR)? getStringFieldF(getObjectFieldF(doc, CSUB_EXPR), CSUB_EXPR_GEOREL) : "",
                                          stringFilterP,
                                          doc.hasField(CSUB_FORMAT)? stringToRenderFormat(getStringFieldF(doc, CSUB_FORMAT)) : NGSI_V2_NORMALIZED);

  if (subCacheP != NULL)
  {
    LM_T(LmtSubCache, ("Calling subCacheItemRemove"));
    subCacheItemRemove(subCacheP);
  }

  if (mscInsert == 0)  // 0: Insertion was really made
  {
    subCacheUpdateStatisticsIncrement();
  }

  cacheSemGive(__FUNCTION__, "Updating cached subscription");


  reqSemGive(__FUNCTION__, "ngsiv2 update subscription request", reqSemTaken);

  return subUp.id;
}
