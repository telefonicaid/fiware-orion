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
#include "common/defaultValues.h"
#include "apiTypesV2/SubscriptionUpdate.h"
#include "rest/OrionError.h"
#include "alarmMgr/alarmMgr.h"
#include "cache/subCache.h"
#include "orionld/common/orionldState.h"             // orionldState

#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonSubscription.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/mongoSubCache.h"
#include "mongoBackend/mongoUpdateSubscription.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONElement;
using mongo::BSONObj;
using mongo::BSONArray;
using mongo::BSONObjBuilder;
using mongo::OID;
using ngsiv2::HttpInfo;
using ngsiv2::Subscription;
using ngsiv2::SubscriptionUpdate;
using ngsiv2::EntID;



/* ****************************************************************************
*
* setExpiration -
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
      double expires = getNumberFieldAsDoubleF(&subOrig, CSUB_EXPIRATION);

      b->append(CSUB_EXPIRATION, expires);
      LM_T(LmtMongo, ("Subscription expiration: %f", expires));
    }
  }
}



/* ****************************************************************************
*
* setHttpInfo -
*/
static void setHttpInfo(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.notificationProvided)
  {
    setHttpInfo(subUp, b);
  }
  else
  {
    // 'reference' is a mandatory field and 'custom' has a clear mapping to false in the case of missing field
    std::string reference = getStringFieldF(&subOrig, CSUB_REFERENCE);
    bool        custom    = subOrig.hasField(CSUB_CUSTOM) ? getBoolFieldF(&subOrig, CSUB_CUSTOM) : false;

    b->append(CSUB_REFERENCE, reference);
    b->append(CSUB_CUSTOM,    custom);

    LM_T(LmtMongo, ("Subscription reference: %s", reference.c_str()));
    LM_T(LmtMongo, ("Subscription custom:    %s", custom? "true" : "false"));

    if (subOrig.hasField(CSUB_METHOD))
    {
      std::string method = getStringFieldF(&subOrig, CSUB_METHOD);

      b->append(CSUB_METHOD, method);
      LM_T(LmtMongo, ("Subscription method: %s", method.c_str()));
    }

    if (subOrig.hasField(CSUB_HEADERS))
    {
      BSONObj headers;
      getObjectFieldF(&headers, &subOrig, CSUB_HEADERS);

      b->append(CSUB_HEADERS, headers);
      LM_T(LmtMongo, ("Subscription headers: %s", headers.toString().c_str()));
    }

    if (subOrig.hasField(CSUB_QS))
    {
      BSONObj qs;
      getObjectFieldF(&qs, &subOrig, CSUB_QS);

      b->append(CSUB_QS, qs);
      LM_T(LmtMongo, ("Subscription qs: %s", qs.toString().c_str()));
    }

    if (subOrig.hasField(CSUB_PAYLOAD))
    {
      std::string payload = getStringFieldF(&subOrig, CSUB_PAYLOAD);

      b->append(CSUB_PAYLOAD, payload);
      LM_T(LmtMongo, ("Subscription payload: %s", payload.c_str()));
    }
  }
}



/* ****************************************************************************
*
* setThrottling -
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
      double throttling = getNumberFieldAsDoubleF(&subOrig, CSUB_THROTTLING);

      b->append(CSUB_THROTTLING, throttling);
      LM_T(LmtMongo, ("Subscription throttling: %f", throttling));
    }
  }
}



/* ****************************************************************************
*
* setDescription -
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
      std::string description = getStringFieldF(&subOrig, CSUB_DESCRIPTION);

      b->append(CSUB_DESCRIPTION, description);
      LM_T(LmtMongo, ("Subscription description: %s", description.c_str()));
    }
  }
}



/* ****************************************************************************
*
* setStatus -
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
      std::string status = getStringFieldF(&subOrig, CSUB_STATUS);

      b->append(CSUB_STATUS, status);
      LM_T(LmtMongo, ("Subscription status: %s", status.c_str()));
    }
  }
}



/* ****************************************************************************
*
* setEntities -
*/
static void setEntities(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.subjectProvided && !subUp.fromNgsiv1)
  {
    //
    // NGSIv1 doesn't allow to change entities,
    // see https://fiware-orion.readthedocs.io/en/master/user/updating_regs_and_subs/index.html
    //
    setEntities(subUp, b);
  }
  else
  {
    //
    // Ignoring bool response from getArrayField
    // If not found, an empty array is added to 'b'
    //
    BSONArray entities;
    getArrayField(&entities, &subOrig, CSUB_ENTITIES, __FILE__, __LINE__);
    b->append(CSUB_ENTITIES, entities);
  }
}



/* ****************************************************************************
*
* setAttrs -
*/
static void setAttrs(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.notificationProvided)
  {
    setAttrs(subUp, b);
  }
  else
  {
    //
    // Ignoring bool response from getArrayField
    // If not found, an empty array is added to 'b'
    //
    BSONArray attrs;
    getArrayField(&attrs, &subOrig, CSUB_ATTRS, __FILE__, __LINE__);
    b->append(CSUB_ATTRS, attrs);
  }
}



/* ****************************************************************************
*
* setCondsAndInitialNotifyNgsiv1 -
*
* This method could be removed along with the rest of NGSIv1 stuff
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
  //
  // Similar to setCondsAndInitialNotify() in MongoCommonSubscripion.cpp, but
  // entities and notifications attributes are not taken from sub but from subOrig
  //
  // Most of the following code is copied from mongoGetSubscription logic
  //

  std::vector<EntID>        entities;
  std::vector<BSONElement>  ents = getFieldF(subOrig, CSUB_ENTITIES).Array();

  for (unsigned int ix = 0; ix < ents.size(); ++ix)
  {
    BSONObj     ent       = ents[ix].embeddedObject();
    std::string id        = getStringFieldF(&ent, CSUB_ENTITY_ID);
    std::string type      = ent.hasField(CSUB_ENTITY_TYPE)? getStringFieldF(&ent, CSUB_ENTITY_TYPE) : "";
    std::string isPattern = getStringFieldF(&ent, CSUB_ENTITY_ISPATTERN);
    EntID       en;

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
  std::vector<std::string> metadata;

  setStringVectorF(subOrig, CSUB_ATTRS, &attributes);

  if (subOrig.hasField(CSUB_METADATA))
  {
    setStringVectorF(subOrig, CSUB_METADATA, &metadata);
  }

  /* Conds vector (and maybe an initial notification) */
  *notificationDone = false;

  BSONArray  conds = processConditionVector(sub.subject.condition.attributes,
                                            entities,
                                            attributes,
                                            metadata,
                                            subId,
                                            ngsiv2::HttpInfo(url),
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
    std::string               status;
    HttpInfo                  httpInfo;
    bool                      blacklist;
    std::vector<std::string>  notifAttributesV;
    std::vector<std::string>  metadataV;
    RenderFormat              attrsFormat = NGSI_V2_NORMALIZED;

    if (subUp.statusProvided)
    {
      status = subUp.status;
    }
    else
    {
      status = subOrig.hasField(CSUB_STATUS)? getStringFieldF(&subOrig, CSUB_STATUS) : STATUS_ACTIVE;
    }

    if (subUp.notificationProvided)
    {
      httpInfo         = subUp.notification.httpInfo;
      blacklist        = subUp.notification.blacklist;
      metadataV        = subUp.notification.metadata;
      notifAttributesV = subUp.notification.attributes;
    }
    else
    {
      httpInfo.fill(&subOrig);
      blacklist = subOrig.hasField(CSUB_BLACKLIST)? getBoolFieldF(&subOrig, CSUB_BLACKLIST) : false;
      setStringVectorF(subOrig, CSUB_ATTRS, &notifAttributesV);

      if (subOrig.hasField(CSUB_METADATA))
      {
        setStringVectorF(subOrig, CSUB_METADATA, &metadataV);
      }
    }

    if (subUp.attrsFormatProvided)
    {
      attrsFormat = subUp.attrsFormat;
    }
    else if (subOrig.hasField(CSUB_FORMAT))
    {
      attrsFormat = stringToRenderFormat(getStringFieldF(&subOrig, CSUB_FORMAT));
    }

    if (subUp.fromNgsiv1)
    {
      //
      // In NGSIv1 is legal updating conditions without updating entities, which is not possible
      // in NGSIv2 (as both entities and coditions are part of 'subject' and they are updated as
      // a whole). In addition, NGSIv1 doesn't allow to update notification attributes. Both
      // (entities and notification attributes) are passed in subOrig.
      //
      // See: https://fiware-orion.readthedocs.io/en/master/user/updating_regs_and_subs/index.html
      //
      setCondsAndInitialNotifyNgsiv1(subUp,
                                     subOrig,
                                     subUp.id,
                                     status,
                                     httpInfo.url,
                                     attrsFormat,
                                     tenant,
                                     servicePathV,
                                     xauthToken,
                                     fiwareCorrelator,
                                     b,
                                     notificationDone);
    }
    else
    {
      setCondsAndInitialNotify(subUp,
                               subUp.id,
                               status,
                               notifAttributesV,
                               metadataV,
                               httpInfo,
                               blacklist,
                               attrsFormat,
                               tenant,
                               servicePathV,
                               xauthToken,
                               fiwareCorrelator,
                               b,
                               notificationDone);
    }
  }
  else
  {
    //
    // Ignoring bool response from getArrayField
    // If not found, an empty array is added to 'b'
    //
    BSONArray conds;
    getArrayField(&conds, &subOrig, CSUB_CONDITIONS, __FILE__, __LINE__);
    b->append(CSUB_CONDITIONS, conds);
  }
}



/* ****************************************************************************
*
* setCount -
*/
static void setCount(long long inc, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subOrig.hasField(CSUB_COUNT))
  {
    long long count = getIntOrLongFieldAsLongF(&subOrig, CSUB_COUNT);
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
* NOTE
*   Unlike setLastFailure() and setLastSucces(), this function doesn't return any value.
*   This is due to the fact that lastNotification is added to before sending the notification
*   while the other two (lastSuccess/lastFailure) need to wait until after - to know the status
*   of the notification and the resulting values are stored in the sub-cache only,
*   to be added to mongo when a sub cache refresh is performed.
*/
static void setLastNotification(const BSONObj& subOrig, CachedSubscription* subCacheP, BSONObjBuilder* b)
{
  //
  // FIXME P1: if CSUB_LASTNOTIFICATION is not in the original doc, it will also not be in the new doc.
  //           Is this necessary?
  //           The implementation would get a lot simpler if we ALWAYS add CSUB_LASTNOTIFICATION and CSUB_COUNT
  //           to 'newSub'
  //
  if (!subOrig.hasField(CSUB_LASTNOTIFICATION))
  {
    return;
  }

  double lastNotification = getNumberFieldAsDoubleF(&subOrig, CSUB_LASTNOTIFICATION);

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
* setLastFailure -
*/
static double setLastFailure(const BSONObj& subOrig, CachedSubscription* subCacheP, BSONObjBuilder* b)
{
  double lastFailure = subOrig.hasField(CSUB_LASTFAILURE)? getNumberFieldAsDoubleF(&subOrig, CSUB_LASTFAILURE) : 0;

  //
  // Compare with 'lastFailure' from the sub-cache.
  // If the cached value of lastFailure is higher, then use it.
  //
  if ((subCacheP != NULL) && (subCacheP->lastFailure > lastFailure))
  {
    lastFailure = subCacheP->lastFailure;
  }

  setLastFailure(lastFailure, b);

  return lastFailure;
}



/* ****************************************************************************
*
* setLastSuccess -
*/
static double setLastSuccess(const BSONObj& subOrig, CachedSubscription* subCacheP, BSONObjBuilder* b)
{
  double lastSuccess = getNumberFieldAsDoubleF(&subOrig, CSUB_LASTSUCCESS);

  //
  // Compare with 'lastSuccess' from the sub-cache.
  // If the cached value of lastSuccess is higher, then use it.
  //
  if ((subCacheP != NULL) && (subCacheP->lastSuccess > lastSuccess))
  {
    lastSuccess = subCacheP->lastSuccess;
  }

  setLastSuccess(lastSuccess, b);

  return lastSuccess;
}



/* ****************************************************************************
*
* setExpression -
*/
static void setExpression(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.subjectProvided)
  {
    setExpression(subUp, b);
  }
  else
  {
    BSONObj expression;

    if (subOrig.hasField(CSUB_EXPR))
    {
      getObjectFieldF(&expression, &subOrig, CSUB_EXPR);
    }
    else
    {
      /* This part of the if clause corresponds to csub that were created before expression was invented
       * (using an old Orion version) which are now being updated. In this case, we introduce an empty
       * expression, but with all the expected fiedls */
      expression = BSON(CSUB_EXPR_Q      << "" <<
                        CSUB_EXPR_MQ     << "" <<
                        CSUB_EXPR_GEOM   << "" <<
                        CSUB_EXPR_COORDS << "" <<
                        CSUB_EXPR_GEOREL << "");
    }
    b->append(CSUB_EXPR, expression);
    LM_T(LmtMongo, ("Subscription expression: %s", expression.toString().c_str()));
  }
}



/* ****************************************************************************
*
* setFormat -
*/
static void setFormat(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.attrsFormatProvided)
  {
    setFormat(subUp, b);
  }
  else
  {
    std::string format = getStringFieldF(&subOrig, CSUB_FORMAT);

    b->append(CSUB_FORMAT, format);
    LM_T(LmtMongo, ("Subscription format: %s", format.c_str()));
  }
}



/* ****************************************************************************
*
* setBlacklist -
*/
static void setBlacklist(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.blacklistProvided)
  {
    setBlacklist(subUp, b);
  }
  else
  {
    bool bList = subOrig.hasField(CSUB_BLACKLIST)? getBoolFieldF(&subOrig, CSUB_BLACKLIST) : false;

    b->append(CSUB_BLACKLIST, bList);
    LM_T(LmtMongo, ("Subscription blacklist: %s", bList? "true" : "false"));
  }
}



/* ****************************************************************************
*
* setMetadata -
*/
static void setMetadata(const SubscriptionUpdate& subUp, const BSONObj& subOrig, BSONObjBuilder* b)
{
  if (subUp.notificationProvided)
  {
    setMetadata(subUp, b);
  }
  else
  {
    //
    // Note that if subOrig doesn't have CSUB_METADATA (e.g. old subscription in the DB created before
    // this feature) BSONArray constructor ensures an empty array
    //

    //
    // Ignoring bool response from getArrayField
    // If not found, an empty array is added to 'b'
    //
    BSONArray metadata;
    getArrayField(&metadata, &subOrig, CSUB_METADATA, __FILE__, __LINE__);
    b->append(CSUB_METADATA, metadata);
  }
}



/* ****************************************************************************
*
* updateInCache -
*/
void updateInCache
(
  const BSONObj&             doc,
  const SubscriptionUpdate&  subUp,
  const std::string&         tenant,
  double                     lastNotification,
  double                     lastFailure,
  double                     lastSuccess
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


  // 0. Lookup matching subscription in subscription-cache

  cacheSemTake(__FUNCTION__, "Updating cached subscription");

  //
  // Second lookup for the same in the mongo update subscription process.
  // However, we have to do it, as the item in the cache could have been changed in the meanwhile.
  //
  LM_T(LmtSubCache, ("update: %s", doc.toString().c_str()));

  CachedSubscription* subCacheP        = subCacheItemLookup(tenant.c_str(), subUp.id.c_str());
  char*               servicePathCache = (char*) ((subCacheP == NULL)? "" : subCacheP->servicePath);
  std::string         q;
  std::string         mq;
  std::string         geom;
  std::string         coords;
  std::string         georel;
  RenderFormat        renderFormat = NGSI_V2_NORMALIZED;  // Default value

  if (doc.hasField(CSUB_FORMAT))
  {
    renderFormat = stringToRenderFormat(getStringFieldF(&doc, CSUB_FORMAT));
  }

  if (doc.hasField(CSUB_EXPR))
  {
    BSONObj expr;
    getObjectFieldF(&expr, &doc, CSUB_EXPR);

    q      = expr.hasField(CSUB_EXPR_Q)?      getStringFieldF(&expr, CSUB_EXPR_Q)      : "";
    mq     = expr.hasField(CSUB_EXPR_MQ)?     getStringFieldF(&expr, CSUB_EXPR_MQ)     : "";
    geom   = expr.hasField(CSUB_EXPR_GEOM)?   getStringFieldF(&expr, CSUB_EXPR_GEOM)   : "";
    coords = expr.hasField(CSUB_EXPR_COORDS)? getStringFieldF(&expr, CSUB_EXPR_COORDS) : "";
    georel = expr.hasField(CSUB_EXPR_GEOREL)? getStringFieldF(&expr, CSUB_EXPR_GEOREL) : "";
  }


  int mscInsert = mongoSubCacheItemInsert(tenant.c_str(),
                                          doc,
                                          subUp.id.c_str(),
                                          servicePathCache,
                                          lastNotification,
                                          lastFailure,
                                          lastSuccess,
                                          doc.hasField(CSUB_EXPIRATION)? getNumberFieldAsDoubleF(&doc, CSUB_EXPIRATION) : 0,
                                          doc.hasField(CSUB_STATUS)? getStringFieldF(&doc, CSUB_STATUS) : STATUS_ACTIVE,
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
  const std::vector<std::string>&  servicePathV,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator
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
    std::string details;

    reqSemGive(__FUNCTION__, "ngsiv2 update subscription request", reqSemTaken);

    if (sc.code == SccContextElementNotFound)
    {
      details = std::string("invalid OID mimeType: '") + subUp.id + "'";
      alarmMgr.badInput(clientIp, details);
    }
    else  // SccReceiverInternalError
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
  BSONObjBuilder      b;
  std::string         servicePath      = servicePathV[0] == "" ? SERVICE_PATH_ALL : servicePathV[0];
  bool                notificationDone = false;
  double              lastNotification = 0;
  double              lastFailure      = 0;
  double              lastSuccess      = 0;
  CachedSubscription* subCacheP        = NULL;

  if (!noCache)
  {
    subCacheP = subCacheItemLookup(tenant.c_str(), subUp.id.c_str());
  }

  setExpiration(subUp, subOrig, &b);
  setHttpInfo(subUp, subOrig, &b);
  setThrottling(subUp, subOrig, &b);
  setServicePath(servicePath, &b);
  setDescription(subUp, subOrig, &b);
  setStatus(subUp, subOrig, &b);
  setEntities(subUp, subOrig, &b);
  setAttrs(subUp, subOrig, &b);
  setMetadata(subUp, subOrig, &b);
  setBlacklist(subUp, subOrig, &b);

  setCondsAndInitialNotify(subUp,
                           subOrig,
                           tenant,
                           servicePathV,
                           xauthToken,
                           fiwareCorrelator,
                           &b,
                           &notificationDone);

  if (notificationDone)
  {
    int64_t countInc = 1;

    lastNotification = orionldState.requestTime;

    // Update sub-cache
    if (subCacheP != NULL)
    {
      cacheSemTake(__FUNCTION__, "Updating count and last notification in cache subscription");

      subCacheP->count                 += 1;  // 'count' to be reset later if DB operation OK
      subCacheP->lastNotificationTime  = lastNotification;

      cacheSemGive(__FUNCTION__, "Updating count and last notification in cache subscription");

      countInc = subCacheP->count;  // already inc with +1
    }

    setLastNotification(lastNotification, &b);
    setCount(countInc, subOrig, &b);
  }
  else
  {
    setLastNotification(subOrig, subCacheP, &b);
    setCount(0, subOrig, &b);
  }

  lastFailure = setLastFailure(subOrig, subCacheP, &b);
  lastSuccess = setLastSuccess(subOrig, subCacheP, &b);

  setExpression(subUp, subOrig, &b);
  setFormat(subUp, subOrig, &b);

  BSONObj doc = b.obj();

  // Update in DB
  std::string  colName = getSubscribeContextCollectionName(tenant);
  BSONObj      bson    = BSON("_id" << OID(subUp.id));

  if (!collectionUpdate(colName, bson, doc, false, &err))
  {
    reqSemGive(__FUNCTION__, "ngsiv2 update subscription request (mongo db exception)", reqSemTaken);
    oe->fill(SccReceiverInternalError, err);

    return "";
  }

  // Update in cache
  if (!noCache)
  {
    updateInCache(doc, subUp, tenant, lastNotification, lastFailure, lastSuccess);
  }

  reqSemGive(__FUNCTION__, "ngsiv2 update subscription request", reqSemTaken);

  return subUp.id;
}
