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

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/sem.h"
#include "common/statistics.h"
#include "common/idCheck.h"
#include "common/errorMessages.h"
#include "rest/ConnectionInfo.h"
#include "cache/subCache.h"
#include "apiTypesV2/Subscription.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonSubscription.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoGetSubscriptions.h"



/* ****************************************************************************
*
* USING -
*/
using mongo::BSONObj;
using mongo::BSONElement;
using mongo::DBClientCursor;
using mongo::DBClientBase;
using mongo::Query;
using mongo::OID;
using ngsiv2::Subscription;
using ngsiv2::EntID;



/* ****************************************************************************
*
* setSubscriptionId -
*/
static void setNewSubscriptionId(Subscription* s, const BSONObj& r)
{
  s->id = getFieldF(r, "_id").OID().toString();
}



/* ****************************************************************************
*
* setDescription -
*/
static void setDescription(Subscription* s, const BSONObj& r)
{
  LM_TMP(("s: %p", s));
  s->description = r.hasField(CSUB_DESCRIPTION) ? getStringFieldF(r, CSUB_DESCRIPTION) : "";
}



/* ****************************************************************************
*
* setSubject -
*/
static void setSubject(Subscription* s, const BSONObj& r)
{
  // Entities
  std::vector<BSONElement> ents = getFieldF(r, CSUB_ENTITIES).Array();
  for (unsigned int ix = 0; ix < ents.size(); ++ix)
  {
    BSONObj ent               = ents[ix].embeddedObject();
    std::string id            = getStringFieldF(ent, CSUB_ENTITY_ID);
    std::string type          = ent.hasField(CSUB_ENTITY_TYPE)? getStringFieldF(ent, CSUB_ENTITY_TYPE) : "";
    std::string isPattern     = getStringFieldF(ent, CSUB_ENTITY_ISPATTERN);
    bool        isTypePattern = ent.hasField(CSUB_ENTITY_ISTYPEPATTERN)?
                                  getBoolFieldF(ent, CSUB_ENTITY_ISTYPEPATTERN) : false;

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
  setStringVectorF(r, CSUB_CONDITIONS, &(s->subject.condition.attributes));

  if (r.hasField(CSUB_EXPR))
  {
    mongo::BSONObj expression = getObjectFieldF(r, CSUB_EXPR);

    std::string  q      = expression.hasField(CSUB_EXPR_Q)      ? getStringFieldF(expression, CSUB_EXPR_Q)      : "";
    std::string  mq     = expression.hasField(CSUB_EXPR_MQ)     ? getStringFieldF(expression, CSUB_EXPR_MQ)     : "";
    std::string  geo    = expression.hasField(CSUB_EXPR_GEOM)   ? getStringFieldF(expression, CSUB_EXPR_GEOM)   : "";
    std::string  coords = expression.hasField(CSUB_EXPR_COORDS) ? getStringFieldF(expression, CSUB_EXPR_COORDS) : "";
    std::string  georel = expression.hasField(CSUB_EXPR_GEOREL) ? getStringFieldF(expression, CSUB_EXPR_GEOREL) : "";

    if (q  != "")      s->subject.condition.expression.q        = q;
    if (mq != "")      s->subject.condition.expression.mq       = mq;
    if (geo != "")     s->subject.condition.expression.geometry = geo;
    if (coords != "")  s->subject.condition.expression.coords   = coords;
    if (georel != "")  s->subject.condition.expression.georel   = georel;
  }
}


/* ****************************************************************************
*
* setNotification -
*/
static void setNotification(Subscription* subP, const BSONObj& r, const std::string& tenant)
{
  // Attributes
  setStringVectorF(r, CSUB_ATTRS, &(subP->notification.attributes));

  // Metadata
  if (r.hasField(CSUB_METADATA))
  {
    setStringVectorF(r, CSUB_METADATA, &(subP->notification.metadata));
  }

  subP->notification.httpInfo.fill(r);

  ngsiv2::Notification* nP = &subP->notification;

  subP->throttling      = r.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(r, CSUB_THROTTLING)       : -1;
  nP->lastNotification  = r.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongF(r, CSUB_LASTNOTIFICATION) : -1;
  nP->timesSent         = r.hasField(CSUB_COUNT)?            getIntOrLongFieldAsLongF(r, CSUB_COUNT)            : -1;
  nP->blacklist         = r.hasField(CSUB_BLACKLIST)?        getBoolFieldF(r, CSUB_BLACKLIST)                   : false;
  nP->lastFailure       = r.hasField(CSUB_LASTFAILURE)?      getIntOrLongFieldAsLongF(r, CSUB_LASTFAILURE)      : -1;
  nP->lastSuccess       = r.hasField(CSUB_LASTSUCCESS)?      getIntOrLongFieldAsLongF(r, CSUB_LASTSUCCESS)      : -1;

  // Attributes format
  subP->attrsFormat = r.hasField(CSUB_FORMAT)? stringToRenderFormat(getStringFieldF(r, CSUB_FORMAT)) : NGSI_V1_LEGACY;


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
      subP->notification.lastFailure = cSubP->lastFailure;
    }

    if (cSubP->lastSuccess > subP->notification.lastSuccess)
    {
      subP->notification.lastSuccess = cSubP->lastSuccess;
    }
  }
  cacheSemGive(__FUNCTION__, "get lastNotification and count");
}



/* ****************************************************************************
*
* setStatus -
*/
static void setStatus(Subscription* s, const BSONObj& r)
{
  s->expires = r.hasField(CSUB_EXPIRATION)? getIntOrLongFieldAsLongF(r, CSUB_EXPIRATION) : -1;

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
    s->status = r.hasField(CSUB_STATUS) ? getStringFieldF(r, CSUB_STATUS) : STATUS_ACTIVE;
  }
  else
  {
    s->status = "expired";
  }
}


#ifdef ORIONLD
/* ****************************************************************************
*
* setSubscriptionId -
*/
static void setSubscriptionId(Subscription* s, const BSONObj& r)
{
  s->id = getStringFieldF(r, "_id");
}



/* ****************************************************************************
*
* setName -
*/
static void setName(Subscription* s, const BSONObj& r)
{
  s->name = r.hasField(CSUB_NAME) ? getStringFieldF(r, CSUB_NAME) : "";
}



/* ****************************************************************************
*
* setContext -
*/
static void setContext(Subscription* s, const BSONObj& r)
{
  s->ldContext = r.hasField(CSUB_LDCONTEXT) ? getStringFieldF(r, CSUB_LDCONTEXT) : "";
}



/* ****************************************************************************
*
* setMimeType -
*/
static void setMimeType(Subscription* s, const BSONObj& r)
{
  if (r.hasField(CSUB_NAME))
  {
    std::string mimeTypeString = getStringFieldF(r, CSUB_MIMETYPE);

    s->notification.httpInfo.mimeType = longStringToMimeType(mimeTypeString);
  }
}

#endif


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
  std::auto_ptr<DBClientCursor>  cursor;
  std::string                    err;
  Query                          q;

  // FIXME P6: This here is a bug ... See #3099 for more info
  if (!servicePath.empty() && (servicePath != "/#"))
  {
    q = Query(BSON(CSUB_SERVICE_PATH << servicePath));
  }

  q.sort(BSON("_id" << 1));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionRangedQuery(connection,
                             getSubscribeContextCollectionName(tenant),
                             q,
                             limit,
                             offset,
                             &cursor,
                             count,
                             &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo List Subscriptions", reqSemTaken);
    *oe = OrionError(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  unsigned int docs = 0;

  while (moreSafe(cursor))
  {
    BSONObj  r;

    if (!nextSafeOrErrorF(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      continue;
    }

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

  releaseMongoConnection(connection);
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
  OID          oid;
  StatusCode   sc;

  if (safeGetSubId(idSub, &oid, &sc) == false)
  {
    *oe = OrionError(sc);
    return;
  }

  reqSemTake(__FUNCTION__, "Mongo Get Subscription", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Subscription"));

  std::auto_ptr<DBClientCursor>  cursor;
  BSONObj                        q     = BSON("_id" << oid);

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, getSubscribeContextCollectionName(tenant), q, &cursor, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
    *oe = OrionError(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  if (moreSafe(cursor))
  {
    BSONObj r;

    if (!nextSafeOrErrorF(cursor, &r, &err))
    {
      releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
      *oe = OrionError(SccReceiverInternalError, std::string("exception in nextSafe(): ") + err.c_str());
      return;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    setNewSubscriptionId(sub, r);
    setDescription(sub, r);
    setSubject(sub, r);
    setNotification(sub, r, tenant);
    setStatus(sub, r);

    if (moreSafe(cursor))
    {
      releaseMongoConnection(connection);
      // Ooops, we expect only one
      LM_T(LmtMongo, ("more than one subscription: '%s'", idSub.c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
      *oe = OrionError(SccConflict);

      return;
    }
  }
  else
  {
    releaseMongoConnection(connection);
    LM_T(LmtMongo, ("subscription not found: '%s'", idSub.c_str()));
    reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
    *oe = OrionError(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_SUBSCRIPTION, ERROR_NOT_FOUND);

    return;
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);

  *oe = OrionError(SccOk);
}



#ifdef ORIONLD
/* ****************************************************************************
*
* mongoGetLdSubscription -
*/
bool mongoGetLdSubscription
(
  ngsiv2::Subscription*  subP,
  const char*            subId,
  const char*            tenant,
  HttpStatusCode*        statusCodeP,
  char**                 detailsP
)
{
  bool                           reqSemTaken = false;
  std::string                    err;
  std::auto_ptr<DBClientCursor>  cursor;
  BSONObj                        q     = BSON("_id" << subId);

  reqSemTake(__FUNCTION__, "Mongo Get Subscription", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Subscription"));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, getSubscribeContextCollectionName(tenant), q, &cursor, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
    *detailsP    = (char*) "Internal Error during DB-query";
    *statusCodeP = SccReceiverInternalError;
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  if (moreSafe(cursor))
  {
    BSONObj r;

    if (!nextSafeOrErrorF(cursor, &r, &err))
    {
      releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
      *detailsP    = (char*) "Runtime Error (exception in nextSafe)";
      *statusCodeP = SccReceiverInternalError;
      return false;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    setSubscriptionId(subP, r);
    setDescription(subP, r);
    setMimeType(subP, r);
    setSubject(subP, r);
    setNotification(subP, r, tenant);
    setStatus(subP, r);
    setName(subP, r);
    setContext(subP, r);

    if (moreSafe(cursor))
    {
      releaseMongoConnection(connection);

      // Ooops, we expected only one
      LM_T(LmtMongo, ("more than one subscription: '%s'", subId));
      reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
      *detailsP    = (char*) "more than one subscription matched";
      *statusCodeP = SccConflict;
      return false;
    }
  }
  else
  {
    releaseMongoConnection(connection);
    LM_T(LmtMongo, ("subscription not found: '%s'", subId));
    reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
    *detailsP    = (char*) "subscription not found";
    *statusCodeP = SccContextElementNotFound;
    return false;
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);

  *statusCodeP = SccOk;
  return true;
}



/* ****************************************************************************
*
* mongoGetLdSubscriptions - 
*/
bool mongoGetLdSubscriptions
(
  ConnectionInfo*                     ciP,
  std::vector<ngsiv2::Subscription>*  subVecP,
  const char*                         tenant,
  long long*                          countP,
  OrionError*                         oeP
)
{
  bool      reqSemTaken = false;
  int       offset      = atoi(ciP->uriParam[URI_PARAM_PAGINATION_OFFSET].c_str());
  int       limit;

  if (ciP->uriParam[URI_PARAM_PAGINATION_LIMIT] != "")
  {
    limit = atoi(ciP->uriParam[URI_PARAM_PAGINATION_LIMIT].c_str());
    if (limit <= 0)
      limit = DEFAULT_PAGINATION_LIMIT_INT;
  }
  else
    limit = DEFAULT_PAGINATION_LIMIT_INT;

  reqSemTake(__FUNCTION__, "Mongo GET Subscriptions", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo GET Subscriptions"));

  /* ONTIMEINTERVAL subscriptions are not part of NGSIv2, so they are excluded.
   * Note that expiration is not taken into account (in the future, a q= query
   * could be added to the operation in order to filter results)
   */
  std::auto_ptr<DBClientCursor>  cursor;
  std::string                    err;
  Query                          q;

  // FIXME P6: This here is a bug ... See #3099 for more info
  if (!ciP->servicePathV[0].empty() && (ciP->servicePathV[0] != "/#"))
  {
    q = Query(BSON(CSUB_SERVICE_PATH << ciP->servicePathV[0]));
  }

  q.sort(BSON("_id" << 1));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionRangedQuery(connection,
                             getSubscribeContextCollectionName(tenant),
                             q,
                             limit,
                             offset,
                             &cursor,
                             countP,
                             &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo List Subscriptions", reqSemTaken);

    oeP->code    = SccReceiverInternalError;
    oeP->details = err;
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  unsigned int docs = 0;

  while (moreSafe(cursor))
  {
    BSONObj  r;

    if (!nextSafeOrErrorF(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      continue;
    }

    docs++;
    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));

    Subscription s;

    setSubscriptionId(&s, r);
    setDescription(&s, r);
    setSubject(&s, r);
    setStatus(&s, r);
    setNotification(&s, r, tenant);

    subVecP->push_back(s);
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo List Subscriptions", reqSemTaken);

  oeP->code = SccOk;
  return true;
}

#endif
