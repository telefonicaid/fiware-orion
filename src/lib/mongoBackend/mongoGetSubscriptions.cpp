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
#include <string>
#include <vector>
#include <map>

#include "mongoBackend/mongoGetSubscriptions.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/sem.h"
#include "common/statistics.h"
#include "common/idCheck.h"
#include "common/errorMessages.h"
#include "cache/subCache.h"
#include "apiTypesV2/Subscription.h"

#include "mongoBackend/MongoGlobal.h"  // getSubscribeContextCollectionName
#include "mongoBackend/dbConstants.h"

#include "mongoDriver/mongoConnectionPool.h"
#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/safeMongo.h"
#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/OID.h"

/* ****************************************************************************
*
* USING - 
*/
using ngsiv2::Subscription;
using ngsiv2::EntID;



/* ****************************************************************************
*
* setSubscriptionId -
*/
static void setNewSubscriptionId(Subscription* s, const orion::BSONObj& r)
{
  s->id = getFieldFF(r, "_id").OID();
}



/* ****************************************************************************
*
* setDescription -
*/
static void setDescription(Subscription* s, const orion::BSONObj& r)
{
  s->description = r.hasField(CSUB_DESCRIPTION) ? getStringFieldFF(r, CSUB_DESCRIPTION) : "";
}



/* ****************************************************************************
*
* setSubject -
*/
static void setSubject(Subscription* s, const orion::BSONObj& r)
{
  // Entities
  std::vector<orion::BSONElement> ents = getFieldFF(r, CSUB_ENTITIES).Array();
  for (unsigned int ix = 0; ix < ents.size(); ++ix)
  {
    orion::BSONObj ent           = ents[ix].embeddedObject();
    std::string    id            = getStringFieldFF(ent, CSUB_ENTITY_ID);
    std::string    type          = ent.hasField(CSUB_ENTITY_TYPE)? getStringFieldFF(ent, CSUB_ENTITY_TYPE) : "";
    std::string    isPattern     = getStringFieldFF(ent, CSUB_ENTITY_ISPATTERN);
    bool           isTypePattern = ent.hasField(CSUB_ENTITY_ISTYPEPATTERN)?
                                     getBoolFieldFF(ent, CSUB_ENTITY_ISTYPEPATTERN) : false;

    EntID en;
    if (isFalse(isPattern))
    {
      en.id = id;
    }
    else
    {
      en.idPattern = id;
    }

    if (!isTypePattern)
    {
      en.type = type;
    }
    else  // isTypePattern
    {
      en.typePattern = type;
    }


    s->subject.entities.push_back(en);
  }

  // Condition
  setStringVectorFF(r, CSUB_CONDITIONS, &(s->subject.condition.attributes));

  if (r.hasField(CSUB_EXPR))
  {
    orion::BSONObj expression = getObjectFieldFF(r, CSUB_EXPR);

    std::string  q      = expression.hasField(CSUB_EXPR_Q)      ? getStringFieldFF(expression, CSUB_EXPR_Q)      : "";
    std::string  mq     = expression.hasField(CSUB_EXPR_MQ)     ? getStringFieldFF(expression, CSUB_EXPR_MQ)     : "";
    std::string  geo    = expression.hasField(CSUB_EXPR_GEOM)   ? getStringFieldFF(expression, CSUB_EXPR_GEOM)   : "";
    std::string  coords = expression.hasField(CSUB_EXPR_COORDS) ? getStringFieldFF(expression, CSUB_EXPR_COORDS) : "";
    std::string  georel = expression.hasField(CSUB_EXPR_GEOREL) ? getStringFieldFF(expression, CSUB_EXPR_GEOREL) : "";

    s->subject.condition.expression.q        = q;
    s->subject.condition.expression.mq       = mq;
    s->subject.condition.expression.geometry = geo;
    s->subject.condition.expression.coords   = coords;
    s->subject.condition.expression.georel   = georel;
  }
}


/* ****************************************************************************
*
* setNotification -
*/
static void setNotification(Subscription* subP, const orion::BSONObj& r, const std::string& tenant)
{
  // Attributes
  setStringVectorFF(r, CSUB_ATTRS, &(subP->notification.attributes));

  // Metadata
  if (r.hasField(CSUB_METADATA))
  {
    setStringVectorFF(r, CSUB_METADATA, &(subP->notification.metadata));
  }

  subP->notification.httpInfo.fill(r);

  ngsiv2::Notification* nP = &subP->notification;

  subP->throttling      = r.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongFF(r, CSUB_THROTTLING)       : -1;
  nP->lastNotification  = r.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongFF(r, CSUB_LASTNOTIFICATION) : -1;
  nP->timesSent         = r.hasField(CSUB_COUNT)?            getIntOrLongFieldAsLongFF(r, CSUB_COUNT)            : -1;
  nP->blacklist         = r.hasField(CSUB_BLACKLIST)?        getBoolFieldFF(r, CSUB_BLACKLIST)                   : false;
  nP->onlyChanged       = r.hasField(CSUB_ONLYCHANGED)?      getBoolFieldFF(r, CSUB_ONLYCHANGED)                 : false;
  nP->lastFailure       = r.hasField(CSUB_LASTFAILURE)?      getIntOrLongFieldAsLongFF(r, CSUB_LASTFAILURE)      : -1;
  nP->lastSuccess       = r.hasField(CSUB_LASTSUCCESS)?      getIntOrLongFieldAsLongFF(r, CSUB_LASTSUCCESS)      : -1;
  nP->lastFailureReason = r.hasField(CSUB_LASTFAILUREASON)?  getStringFieldFF(r, CSUB_LASTFAILUREASON)           : "";
  nP->lastSuccessCode   = r.hasField(CSUB_LASTSUCCESSCODE)?  getIntOrLongFieldAsLongFF(r, CSUB_LASTSUCCESSCODE)  : -1;

  // Attributes format
  subP->attrsFormat = r.hasField(CSUB_FORMAT)? stringToRenderFormat(getStringFieldFF(r, CSUB_FORMAT)) : NGSI_V1_LEGACY;


  //
  // Check values from subscription cache, update object from cache-values if necessary
  //
  // NOTE: only 'lastNotificationTime' and 'count'
  //
  cacheSemTake(__FUNCTION__, "get lastNotification and count");
  CachedSubscription* cSubP = subCacheItemLookup(tenant.c_str(), subP->id.c_str());
  if (cSubP)
  {
    if (cSubP->lastNotificationTime > subP->notification.lastNotification)
    {
      subP->notification.lastNotification = cSubP->lastNotificationTime;
    }

    if (cSubP->count != 0)
    {
      //
      // First, compensate for -1 in 'timesSent'
      //
      if (subP->notification.timesSent == -1)
      {
        subP->notification.timesSent = 0;
      }

      subP->notification.timesSent += cSubP->count;
    }

    if (cSubP->lastFailure > subP->notification.lastFailure)
    {
      subP->notification.lastFailure       = cSubP->lastFailure;
      subP->notification.lastFailureReason = cSubP->lastFailureReason;
    }

    if (cSubP->lastSuccess > subP->notification.lastSuccess)
    {
      subP->notification.lastSuccess     = cSubP->lastSuccess;
      subP->notification.lastSuccessCode = cSubP->lastSuccessCode;
    }
  }
  cacheSemGive(__FUNCTION__, "get lastNotification and count");
}



/* ****************************************************************************
*
* setStatus -
*/
static void setStatus(Subscription* s, const orion::BSONObj& r)
{
  s->expires = r.hasField(CSUB_EXPIRATION)? getIntOrLongFieldAsLongFF(r, CSUB_EXPIRATION) : -1;

  //
  // Status
  // FIXME P10: use an enum for active/inactive/expired
  //
  // NOTE:
  //   if the field CSUB_EXPIRATION is not present in the subscription, then the default
  //   value of "-1 == never expires" is used.
  //
  if ((s->expires > getCurrentTime()) || (s->expires == -1))
  {
    s->status = r.hasField(CSUB_STATUS) ? getStringFieldFF(r, CSUB_STATUS) : STATUS_ACTIVE;
  }
  else
  {
    s->status = "expired";
  }
}



/* ****************************************************************************
*
* mongoListSubscriptions -
*/
void mongoListSubscriptions
(
  std::vector<Subscription>*           subs,
  OrionError*                          oe,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   tenant,
  const std::string&                   servicePath,  // FIXME P4: vector of strings and not just a single string? See #3100
  int                                  limit,
  int                                  offset,
  long long*                           count
)
{
  bool  reqSemTaken = false;

  reqSemTake(__FUNCTION__, "Mongo List Subscriptions", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo List Subscriptions"));

  /* ONTIMEINTERVAL subscriptions are not part of NGSIv2, so they are excluded.
   * Note that expiration is not taken into account (in the future, a q= query
   * could be added to the operation in order to filter results)
   */
  orion::DBCursor        cursor;
  std::string            err;
  orion::BSONObjBuilder  qB;
  orion::BSONObjBuilder  sortBy;

  // FIXME P6: This here is a bug ... See #3099 for more info
  if (!servicePath.empty() && (servicePath != "/#"))
  {
    qB.append(CSUB_SERVICE_PATH, servicePath);
  }
  sortBy.append("_id", 1);

  orion::BSONObj q = qB.obj();

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();
  if (!orion::collectionRangedQuery(connection,
                                    getSubscribeContextCollectionName(tenant),
                                    q,
                                    sortBy.obj(),
                                    limit,
                                    offset,
                                    &cursor,
                                    count,
                                    &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo List Subscriptions", reqSemTaken);
    *oe = OrionError(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  unsigned int docs = 0;

  orion::BSONObj  r;
  while (cursor.next(&r))
  {
    /* FIXME OLD-DR: remove?
    if (!nextSafeOrErrorFF(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      continue;
    }
    */

    docs++;
    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));

    Subscription  s;

    setNewSubscriptionId(&s, r);
    setDescription(&s, r);
    setSubject(&s, r);
    setStatus(&s, r);
    setNotification(&s, r, tenant);

    subs->push_back(s);
  }

  orion::releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo List Subscriptions", reqSemTaken);

  *oe = OrionError(SccOk);
}



/* ****************************************************************************
*
* mongoGetSubscription -
*/
void mongoGetSubscription
(
  ngsiv2::Subscription*               sub,
  OrionError*                         oe,
  const std::string&                  idSub,
  std::map<std::string, std::string>& uriParam,
  const std::string&                  tenant
)
{
  bool         reqSemTaken = false;
  std::string  err;
  orion::OID   oid;
  StatusCode   sc;

  if (safeGetSubId(idSub, &oid, &sc) == false)
  {
    *oe = OrionError(sc);
    return;
  }

  reqSemTake(__FUNCTION__, "Mongo Get Subscription", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Subscription"));

  orion::DBCursor        cursor;
  orion::BSONObjBuilder  qB;

  qB.append("_id", oid);
  orion::BSONObj q = qB.obj();

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();
  if (!orion::collectionQuery(connection, getSubscribeContextCollectionName(tenant), q, &cursor, &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
    *oe = OrionError(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  orion::BSONObj r;
  if (cursor.next(&r))
  {
    /* FIXME OLD-DR: remove?
    if (!nextSafeOrErrorFF(cursor, &r, &err))
    {
      orion::releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
      *oe = OrionError(SccReceiverInternalError, std::string("exception in nextSafe(): ") + err.c_str());
      return;
    }*/
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    setNewSubscriptionId(sub, r);
    setDescription(sub, r);
    setSubject(sub, r);
    setNotification(sub, r, tenant);
    setStatus(sub, r);

    /* FIXME OLD-DR: excesive checing. By defition _id is unique, you will not get more than one
    if (orion::moreSafe(&cursor))
    {
      orion::releaseMongoConnection(connection);
      // Ooops, we expect only one
      LM_T(LmtMongo, ("more than one subscription: '%s'", idSub.c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
      *oe = OrionError(SccConflict);

      return;
    }*/
  }
  else
  {
    orion::releaseMongoConnection(connection);
    LM_T(LmtMongo, ("subscription not found: '%s'", idSub.c_str()));
    reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
    *oe = OrionError(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_SUBSCRIPTION, ERROR_NOT_FOUND);

    return;
  }

  orion::releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);

  *oe = OrionError(SccOk);
}
