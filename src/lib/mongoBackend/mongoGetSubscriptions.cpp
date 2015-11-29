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

#include "common/sem.h"
#include "common/statistics.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "mongoBackend/mongoGetSubscriptions.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoSubCache.h"

#include "mongo/client/dbclient.h"

using namespace ngsiv2;



/* ****************************************************************************
*
* setSubscriptionId -
*/
static void setSubscriptionId(Subscription* s, const BSONObj& r)
{
  s->id = getField(r, "_id").OID().toString();
}



/* ****************************************************************************
*
* setSubject -
*/
static void setSubject(Subscription* s, const BSONObj& r)
{
  // Entities
  std::vector<BSONElement> ents = getField(r, CSUB_ENTITIES).Array();
  for (unsigned int ix = 0; ix < ents.size(); ++ix)
  {
    BSONObj ent           = ents[ix].embeddedObject();
    std::string id        = getStringField(ent, CSUB_ENTITY_ID);
    std::string type      = ent.hasField(CSUB_ENTITY_TYPE)? getStringField(ent, CSUB_ENTITY_TYPE) : "";
    std::string isPattern = getStringField(ent, CSUB_ENTITY_ISPATTERN);

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

    s->subject.entities.push_back(en);
  }

  // Condition
  std::vector<BSONElement> conds = getField(r, CSUB_CONDITIONS).Array();
  for (unsigned int ix = 0; ix < conds.size(); ++ix)
  {
    BSONObj cond = conds[ix].embeddedObject();
    // The ONCHANGE check is needed, as a subscription could mix different conditions types in DB
    if (std::string(getStringField(cond, CSUB_CONDITIONS_TYPE)) == "ONCHANGE")
    {
      std::vector<BSONElement> condValues = getField(cond, CSUB_CONDITIONS_VALUE).Array();
      for (unsigned int jx = 0; jx < condValues.size(); ++jx)
      {
        std::string attr = condValues[jx].String();
        s->subject.condition.attributes.push_back(attr);
      }
    }
  }

  // Note that current DB model is based on NGSIv1 and doesn't consider expressions. Thus
  // subject.condition.expression cannot be filled. The implemetion will be enhanced once
  // the DB model gets defined
  // TBD
}


/* ****************************************************************************
*
* setNotification -
*/
static void setNotification(Subscription* s, const BSONObj& r, const std::string& tenant)
{
  // Attributes
  std::vector<BSONElement> attrs = getField(r, CSUB_ATTRS).Array();
  for (unsigned int ix = 0; ix < attrs.size(); ++ix)
  {
    std::string attr = attrs[ix].String();

    s->notification.attributes.push_back(attr);
  }

  s->notification.callback         = getStringField(r, CSUB_REFERENCE);
  s->notification.throttling       = r.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLong(r, CSUB_THROTTLING)       : -1;
  s->notification.lastNotification = r.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLong(r, CSUB_LASTNOTIFICATION) : -1;
  s->notification.timesSent        = r.hasField(CSUB_COUNT)?            getField(r, CSUB_COUNT).numberLong()              : -1;

  //
  // Check values from subscription cache, update object from cache-values if necessary
  //
  CachedSubscription* cSubP = mongoSubCacheItemLookup(tenant.c_str(), s->id.c_str());
  if (cSubP)
  {
    if (cSubP->lastNotificationTime > s->notification.lastNotification)
    {
      s->notification.lastNotification = cSubP->lastNotificationTime;
    }

    if (cSubP->count != 0)
    {
      //
      // First, compensate for -1 in 'timesSent'
      //
      if (s->notification.timesSent == -1)
      {
        s->notification.timesSent = 0;
      }

      s->notification.timesSent += cSubP->count;
    }
  }
}



/* ****************************************************************************
*
* setExpires -
*/
static void setExpires(Subscription* s, const BSONObj& r)
{
  s->expires = r.hasField(CSUB_EXPIRATION)? getIntOrLongFieldAsLong(r, CSUB_EXPIRATION) : -1;

  //
  // Status
  // FIXME P10: use an enum for active/expired
  //
  // NOTE:
  //   if the field CSUB_EXPIRATION is not present in the subscription, then the default
  //   value of "-1 == never expires" is used.
  //
  if ((s->expires > getCurrentTime()) || (s->expires == -1))
  {
    s->status = "active";
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
  int                                  limit,
  int                                  offset,
  long long*                           count
)
{
  bool  reqSemTaken = false;

  reqSemTake(__FUNCTION__, "Mongo List Subscriptions", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo List Subscriptions"));

  /* ONTIMEINTERVAL subscription are not part of NGSIv2, so they are excluded.
   * Note that expiration is not taken into account (in the future, a q= query
   * could be added to the operation in order to filter results) */
  std::auto_ptr<DBClientCursor>  cursor;
  std::string                    err;
  std::string                    conds = std::string(CSUB_CONDITIONS) + "." + CSUB_CONDITIONS_TYPE;
  Query                          q     = Query(BSON(conds << "ONCHANGE"));

  q.sort(BSON("_id" << 1));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionRangedQuery(connection, getSubscribeContextCollectionName(tenant), q, limit, offset, &cursor, count, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo List Subscriptions", reqSemTaken);
    *oe = OrionError(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  while (moreSafe(cursor))
  {
    BSONObj r;    

    if (!nextSafeOrError(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s", err.c_str()));
      continue;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    Subscription  s;

    setSubscriptionId(&s, r);
    setSubject(&s, r);
    setExpires(&s, r);
    setNotification(&s, r, tenant);

    subs->push_back(s);
  }
  releaseMongoConnection(connection);

  reqSemGive(__FUNCTION__, "Mongo List Subscriptions", reqSemTaken);
  *oe = OrionError(SccOk);
  return;
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
  bool  reqSemTaken = false;

  reqSemTake(__FUNCTION__, "Mongo Get Subscription", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Subscription"));

  std::auto_ptr<DBClientCursor>  cursor;
  std::string                    err;
  BSONObj                        q     = BSON("_id" << OID(idSub));

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
    if (!nextSafeOrError(cursor, &r, &err))
    {
      releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s", err.c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
      *oe = OrionError(SccReceiverInternalError, std::string("exception in nextSafe(): ") + err.c_str());
      return;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    setSubscriptionId(sub, r);
    setSubject(sub, r);
    setNotification(sub, r, tenant);
    setExpires(sub, r);

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
    *oe = OrionError(SccSubscriptionIdNotFound);
    return;
  }
  releaseMongoConnection(connection);

  reqSemGive(__FUNCTION__, "Mongo Get Subscription", reqSemTaken);
  *oe = OrionError(SccOk);
  return;
}
