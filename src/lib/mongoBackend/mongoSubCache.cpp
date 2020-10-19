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
* Author: Ken Zangelin
*/
#include <regex.h>
#include <string>
#include <vector>

#include "mongoBackend/mongoSubCache.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "common/string.h"
#include "common/statistics.h"
#include "common/RenderFormat.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/StringFilter.h"
#include "cache/subCache.h"

#include "mongoBackend/MongoGlobal.h"  // tenantFromDb (FIXME OLD-DR)
#include "mongoBackend/dbConstants.h"

#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/BSONArrayBuilder.h"
#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/mongoConnectionPool.h"
#include "mongoDriver/safeMongo.h"



/* ****************************************************************************
*
* mongoSubCacheItemInsert -
*
* RETURN VALUES
*   0:  all OK
*  -1:  Database Error - id-field not found
*  -2:  Out of memory (either returns -2 or exit the entire broker)
*  -3:  No patterned entity found
*  -5:  Error parsing string filter
*  -6:  Error parsing metadata string filter
*
* Note that the 'count' of the inserted subscription is set to ZERO.
*
*/
int mongoSubCacheItemInsert(const char* tenant, const orion::BSONObj& sub)
{
  //
  // 01. Check validity of subP parameter
  //
  orion::BSONElement  idField = getFieldFF(sub, "_id");

  if (idField.eoo() == true)
  {
    std::string details = std::string("error retrieving _id field in doc: '") + sub.toString() + "'";
    alarmMgr.dbError(details);
    return -1;
  }
  alarmMgr.dbErrorReset();

  //
  // 03. Create CachedSubscription
  //
  CachedSubscription* cSubP = new CachedSubscription();
  LM_T(LmtSubCache,  ("allocated CachedSubscription at %p", cSubP));

  if (cSubP == NULL)
  {
    // FIXME P7: See github issue #1362
    LM_X(1, ("Runtime Error (cannot allocate memory for a cached subscription: %s)", strerror(errno)));
    return -2;
  }


  //
  // 04. Extract data from subP
  //
  // NOTE: NGSIv1 JSON is 'default' (for old db-content)
  //
  std::string    renderFormatString = sub.hasField(CSUB_FORMAT)? getStringFieldFF(sub, CSUB_FORMAT) : "legacy";
  RenderFormat   renderFormat       = stringToRenderFormat(renderFormatString);

  cSubP->tenant                = (tenant[0] == 0)? strdup("") : strdup(tenant);
  cSubP->subscriptionId        = strdup(idField.OID().c_str());
  cSubP->servicePath           = strdup(sub.hasField(CSUB_SERVICE_PATH)? getStringFieldFF(sub, CSUB_SERVICE_PATH).c_str() : "/");
  cSubP->renderFormat          = renderFormat;
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongFF(sub, CSUB_THROTTLING)       : -1;
  cSubP->expirationTime        = sub.hasField(CSUB_EXPIRATION)?       getIntOrLongFieldAsLongFF(sub, CSUB_EXPIRATION)       : 0;
  cSubP->lastNotificationTime  = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongFF(sub, CSUB_LASTNOTIFICATION) : -1;
  cSubP->status                = sub.hasField(CSUB_STATUS)?           getStringFieldFF(sub, CSUB_STATUS)                    : "active";
  cSubP->blacklist             = sub.hasField(CSUB_BLACKLIST)?        getBoolFieldFF(sub, CSUB_BLACKLIST)                   : false;
  cSubP->lastFailure           = sub.hasField(CSUB_LASTFAILURE)?      getIntOrLongFieldAsLongFF(sub, CSUB_LASTFAILURE)      : -1;
  cSubP->lastSuccess           = sub.hasField(CSUB_LASTSUCCESS)?      getIntOrLongFieldAsLongFF(sub, CSUB_LASTSUCCESS)      : -1;
  cSubP->lastFailureReason     = sub.hasField(CSUB_LASTFAILUREASON)?  getStringFieldFF(sub, CSUB_LASTFAILUREASON)           : "";
  cSubP->lastSuccessCode       = sub.hasField(CSUB_LASTSUCCESSCODE)?  getIntOrLongFieldAsLongFF(sub, CSUB_LASTSUCCESSCODE)  : -1;
  cSubP->count                 = 0;
  cSubP->next                  = NULL;


  //
  // 04.2 httpInfo
  //
  // Note that the URL of the notification is stored outside the httpInfo object in mongo
  //
  cSubP->httpInfo.fill(sub);


  //
  // 04.3 expression
  //
  if (sub.hasField(CSUB_EXPR))
  {
    orion::BSONObj expression = getObjectFieldFF(sub, CSUB_EXPR);

    if (expression.hasField(CSUB_EXPR_Q))
    {
      std::string errorString;

      cSubP->expression.q = getStringFieldFF(expression, CSUB_EXPR_Q);
      if (!cSubP->expression.q.empty())
      {
        if (!cSubP->expression.stringFilter.parse(cSubP->expression.q.c_str(), &errorString))
        {
          LM_E(("Runtime Error (error parsing string filter: %s)", errorString.c_str()));
          subCacheItemDestroy(cSubP);
          delete cSubP;
          return -5;
        }
      }
    }

    if (expression.hasField(CSUB_EXPR_MQ))
    {
      std::string errorString;

      cSubP->expression.mq = getStringFieldFF(expression, CSUB_EXPR_MQ);
      if (!cSubP->expression.mq.empty())
      {
        if (!cSubP->expression.mdStringFilter.parse(cSubP->expression.mq.c_str(), &errorString))
        {
          LM_E(("Runtime Error (error parsing md string filter: %s)", errorString.c_str()));
          subCacheItemDestroy(cSubP);
          delete cSubP;
          return -6;
        }
      }
    }

    if (expression.hasField(CSUB_EXPR_GEOM))
    {
      cSubP->expression.geometry = getStringFieldFF(expression, CSUB_EXPR_GEOM);
    }

    if (expression.hasField(CSUB_EXPR_COORDS))
    {
      cSubP->expression.coords = getStringFieldFF(expression, CSUB_EXPR_COORDS);
    }

    if (expression.hasField(CSUB_EXPR_GEOREL))
    {
      cSubP->expression.georel = getStringFieldFF(expression, CSUB_EXPR_GEOREL);
    }
  }


  //
  // 05. Push Entity-data names to EntityInfo Vector (cSubP->entityInfos)
  //
  std::vector<orion::BSONElement>  eVec = getFieldFF(sub, CSUB_ENTITIES).Array();
  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    orion::BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField(CSUB_ENTITY_ID))
    {
      LM_W(("Runtime Error (got a subscription without id)"));
      continue;
    }

    std::string id            = getStringFieldFF(entity, ENT_ENTITY_ID);
    std::string isPattern     = entity.hasField(CSUB_ENTITY_ISPATTERN)? getStringFieldFF(entity, CSUB_ENTITY_ISPATTERN) : "false";
    std::string type          = entity.hasField(CSUB_ENTITY_TYPE)?      getStringFieldFF(entity, CSUB_ENTITY_TYPE)      : "";
    bool        isTypePattern = entity.hasField(CSUB_ENTITY_ISTYPEPATTERN)? getBoolFieldFF(entity, CSUB_ENTITY_ISTYPEPATTERN) : false;
    EntityInfo* eiP           = new EntityInfo(id, type, isPattern, isTypePattern);

    cSubP->entityIdInfos.push_back(eiP);
  }

  if (cSubP->entityIdInfos.size() == 0)
  {
    LM_E(("ERROR (no patterned entityId) - cleaning up"));
    subCacheItemDestroy(cSubP);
    delete cSubP;
    return -3;
  }


  //
  // 06. Push attribute names to Attribute Vector (cSubP->attributes)
  //
  setStringVectorFF(sub, CSUB_ATTRS, &(cSubP->attributes));

  //
  // 07. Push metadata names to Metadata Vector (cSubP->metadatas)
  //
  if (sub.hasField(CSUB_METADATA))
  {
    setStringVectorFF(sub, CSUB_METADATA, &(cSubP->metadata));
  }

  //
  // 08. Fill in cSubP->notifyConditionV from condVec
  //
  setStringVectorFF(sub, CSUB_CONDITIONS, &(cSubP->notifyConditionV));


  subCacheItemInsert(cSubP);

  return 0;
}



/* ****************************************************************************
*
* mongoSubCacheItemInsert -
*
* RETURN VALUE
*   0: OK - patterned subscription has been inserted
*  -1: Empty subscriptionId
*  -2: Empty servicePath
*  -3: Out of memory (either this or EXIT)
*  -4: Subscription not valid for sub-cache (no entity ids)
*
*
* Note that the 'count' of the inserted subscription is set to ZERO.
* This is because the sub cache only counts the increments in these accumulating counters,
* so that other CBs, operating on the same DB will not overwrite the value of these accumulators
*/
int mongoSubCacheItemInsert
(
  const char*            tenant,
  const orion::BSONObj&  sub,
  const char*            subscriptionId,
  const char*            servicePath,
  int                    lastNotificationTime,
  long long              expirationTime,
  const std::string&     status,
  const std::string&     q,
  const std::string&     mq,
  const std::string&     geometry,
  const std::string&     coords,
  const std::string&     georel,
  StringFilter*          stringFilterP,
  StringFilter*          mdStringFilterP,
  RenderFormat           renderFormat
)
{
  //
  // 01. Check incoming parameters
  //
  if ((subscriptionId == NULL) || (subscriptionId[0] == 0))
  {
    return -1;
  }

  if ((servicePath == NULL) || (servicePath[0] == 0))
  {
    return -2;
  }


  //
  // 02. Allocate CachedSubscription
  //
  CachedSubscription* cSubP = new CachedSubscription();

  LM_T(LmtSubCache,  ("allocated CachedSubscription at %p", cSubP));

  if (cSubP == NULL)
  {
    // FIXME P7: See github issue #1362
    LM_X(1, ("Runtime Error (cannot allocate memory for a cached subscription: %s)", strerror(errno)));
    return -3;
  }


  //
  // 03. Push Entity-data names to EntityInfo Vector (cSubP->entityInfos)
  //     NOTE that if there is no patterned entity in the entity-vector,
  //     then the subscription is not valid for the sub-cache and is rejected.
  //
  std::vector<orion::BSONElement>  eVec = getFieldFF(sub, CSUB_ENTITIES).Array();

  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    orion::BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField(CSUB_ENTITY_ID))
    {
      LM_W(("Runtime Error (got a subscription without id)"));
      continue;
    }

    std::string id            = getStringFieldFF(entity, ENT_ENTITY_ID);
    std::string isPattern     = entity.hasField(CSUB_ENTITY_ISPATTERN)? getStringFieldFF(entity, CSUB_ENTITY_ISPATTERN) : "false";
    std::string type          = entity.hasField(CSUB_ENTITY_TYPE)?      getStringFieldFF(entity, CSUB_ENTITY_TYPE)      : "";
    bool        isTypePattern = entity.hasField(CSUB_ENTITY_ISTYPEPATTERN)? getBoolFieldFF(entity, CSUB_ENTITY_ISTYPEPATTERN) : false;
    EntityInfo* eiP           = new EntityInfo(id, type, isPattern, isTypePattern);

    cSubP->entityIdInfos.push_back(eiP);
  }

  if (cSubP->entityIdInfos.size() == 0)
  {
    subCacheItemDestroy(cSubP);
    delete cSubP;
    return -4;
  }


  //
  // 04. Extract data from mongo sub
  //
  if ((lastNotificationTime == -1) && (sub.hasField(CSUB_LASTNOTIFICATION)))
  {
    //
    // If no lastNotificationTime is given to this function AND
    // if the database objuect contains lastNotificationTime,
    // then use the value from the database
    //
    lastNotificationTime = getIntOrLongFieldAsLongFF(sub, CSUB_LASTNOTIFICATION);
  }

  cSubP->tenant                = (tenant[0] == 0)? NULL : strdup(tenant);
  cSubP->subscriptionId        = strdup(subscriptionId);
  cSubP->servicePath           = strdup(servicePath);
  cSubP->renderFormat          = renderFormat;
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)? getIntOrLongFieldAsLongFF(sub, CSUB_THROTTLING) : -1;
  cSubP->expirationTime        = expirationTime;
  cSubP->lastNotificationTime  = lastNotificationTime;
  cSubP->count                 = 0;
  cSubP->status                = status;
  cSubP->expression.q          = q;
  cSubP->expression.mq         = mq;
  cSubP->expression.geometry   = geometry;
  cSubP->expression.coords     = coords;
  cSubP->expression.georel     = georel;
  cSubP->next                  = NULL;
  cSubP->blacklist             = sub.hasField(CSUB_BLACKLIST)? getBoolFieldFF(sub, CSUB_BLACKLIST) : false;

  //
  // httpInfo
  //
  // Note that the URL of the notification is stored outside the httpInfo object in mongo
  //
  cSubP->httpInfo.fill(sub);


  //
  // String Filters
  //
  std::string errorString;

  if (stringFilterP != NULL)
  {
    //
    // NOTE (for both 'q' and 'mq' string filters)
    //   Here, the subscription should have a String Filter but if fill() fails, it won't.
    //   The subscription is already in mongo and hopefully this erroneous situation is fixed
    //   once the sub-cache is refreshed.
    //
    //   This 'but' should be minimized once the issue 2082 gets implemented.
    //   [ Only reason for fill() to fail (apart from out-of-memory) seems to be an invalid regex ]
    //
    cSubP->expression.stringFilter.fill(stringFilterP, &errorString);
  }

  if (mdStringFilterP != NULL)
  {
    cSubP->expression.mdStringFilter.fill(mdStringFilterP, &errorString);
  }

  LM_T(LmtSubCache, ("set lastNotificationTime to %lu for '%s' (from DB)", cSubP->lastNotificationTime, cSubP->subscriptionId));


  //
  // 06. Push attribute names to Attribute Vector (cSubP->attributes)
  //
  setStringVectorFF(sub, CSUB_ATTRS, &(cSubP->attributes));

  //
  // 07. Push metadata names to Metadata Vector (cSubP->metadatas)
  //
  if (sub.hasField(CSUB_METADATA))
  {
    setStringVectorFF(sub, CSUB_METADATA, &(cSubP->metadata));
  }

  //
  // 08. Fill in cSubP->notifyConditionV from condVec
  //
  setStringVectorFF(sub, CSUB_CONDITIONS, &(cSubP->notifyConditionV));

  subCacheItemInsert(cSubP);

  return 0;
}



/* ****************************************************************************
*
* mongoSubCacheRefresh -
*
* 1. Empty cache
* 2. Lookup all subscriptions in the database
* 3. Insert them again in the cache (with fresh data from database)
*
* NOTE
*   The query for the database ONLY extracts the interesting subscriptions:
*   - "conditions.type" << "ONCHANGE"
*
*   I.e. the subscriptions is for ONCHANGE.
*/
void mongoSubCacheRefresh(const std::string& database)
{
  LM_T(LmtSubCache, ("Refreshing subscription cache for DB '%s'", database.c_str()));

  orion::BSONObj   query;      // empty query (all subscriptions)
  std::string      db          = database;
  std::string      tenant      = tenantFromDb(db);
  std::string      collection  = getSubscribeContextCollectionName(tenant);
  orion::DBCursor  cursor;
  std::string      errorString;

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();
  if (orion::collectionQuery(connection, collection, query, &cursor, &errorString) != true)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  int subNo = 0;
  while (orion::moreSafe(&cursor))
  {
    orion::BSONObj  sub;
    std::string     err;

    if (!nextSafeOrErrorFF(cursor, &sub, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      continue;
    }

    int r = mongoSubCacheItemInsert(tenant.c_str(), sub);
    if (r == 0)
    {
      ++subNo;
    }
  }
  orion::releaseMongoConnection(connection);

  LM_T(LmtSubCache, ("Added %d subscriptions for database '%s'", subNo, database.c_str()));
}



/* ****************************************************************************
*
* mongoSubCountersUpdateCount -
*/
static void mongoSubCountersUpdateCount
(
  const std::string&  collection,
  const std::string&  subId,
  long long           count
)
{
  orion::BSONObjBuilder  condition;
  orion::BSONObjBuilder  update;
  orion::BSONObjBuilder  countB;
  std::string  err;

  condition.append("_id", orion::OID(subId));
  countB.append(CSUB_COUNT, count);
  update.append("$inc", countB.obj());

  if (collectionUpdate(collection, condition.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Internal Error (error updating 'count' for a subscription)"));
  }
}



/* ****************************************************************************
*
* mongoSubCountersUpdateLastNotificationTime -
*/
static void mongoSubCountersUpdateLastNotificationTime
(
  const std::string&  collection,
  const std::string&  subId,
  long long           lastNotificationTime
)
{
  std::string  err;

  // FIXME OLD-DR: previously this part was based in streamming construction instead of append()
  // should be changed?

  // condition
  orion::BSONObjBuilder condition;

  orion::BSONArrayBuilder  o;

  // first or token
  orion::BSONObjBuilder  or1;
  orion::BSONObjBuilder  lastNotificationTimeFilter;
  lastNotificationTimeFilter.append("$lt", lastNotificationTime);
  or1.append(CSUB_LASTNOTIFICATION, lastNotificationTimeFilter.obj());

  // second or token
  orion::BSONObjBuilder  or2;
  orion::BSONObjBuilder  exists;
  exists.append("$exists", false);
  or2.append(CSUB_LASTNOTIFICATION, exists.obj());

  o.append(or1.obj());
  o.append(or2.obj());

  condition.append("_id", orion::OID(subId));
  condition.append("$or", o.arr());

  // update
  orion::BSONObjBuilder update;

  orion::BSONObjBuilder lastNotificationTimeB;
  lastNotificationTimeB.append(CSUB_LASTNOTIFICATION, lastNotificationTime);

  update.append("$set", lastNotificationTimeB.obj());

  if (orion::collectionUpdate(collection, condition.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Internal Error (error updating 'lastNotification' for a subscription)"));
  }
}



/* ****************************************************************************
*
* mongoSubCountersUpdateLastFailure -
*/
static void mongoSubCountersUpdateLastFailure
(
  const std::string&  collection,
  const std::string&  subId,
  long long           lastFailure,
  const std::string&  failureReason
)
{
  std::string  err;

  // FIXME OLD-DR: previously this part was based in streamming construction instead of append()
  // should be changed?

  // condition
  orion::BSONObjBuilder condition;

  orion::BSONArrayBuilder  o;

  // first or token
  orion::BSONObjBuilder  or1;
  orion::BSONObjBuilder  lastFailureFilter;
  lastFailureFilter.append("$lt", lastFailure);
  or1.append(CSUB_LASTFAILURE, lastFailureFilter.obj());

  // second or token
  orion::BSONObjBuilder  or2;
  orion::BSONObjBuilder  exists;
  exists.append("$exists", false);
  or2.append(CSUB_LASTFAILURE, exists.obj());

  o.append(or1.obj());
  o.append(or2.obj());

  condition.append("_id", orion::OID(subId));
  condition.append("$or", o.arr());

  // update
  orion::BSONObjBuilder update;

  orion::BSONObjBuilder lastFailureB;
  lastFailureB.append(CSUB_LASTFAILURE, lastFailure);
  lastFailureB.append(CSUB_LASTFAILUREASON, failureReason);

  update.append("$set", lastFailureB.obj());

  if (orion::collectionUpdate(collection, condition.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Internal Error (error updating 'lastFailure' for a subscription)"));
  }
}



/* ****************************************************************************
*
* mongoSubCountersUpdateLastSuccess -
*/
static void mongoSubCountersUpdateLastSuccess
(
  const std::string&  collection,
  const std::string&  subId,
  long long           lastSuccess,
  long long           statusCode
)
{
  std::string  err;

  // FIXME OLD-DR: previously this part was based in streamming construction instead of append()
  // should be changed?

  // condition
  orion::BSONObjBuilder condition;

  orion::BSONArrayBuilder  o;

  // first or token
  orion::BSONObjBuilder  or1;
  orion::BSONObjBuilder  lastSuccessFilter;
  lastSuccessFilter.append("$lt", lastSuccess);
  or1.append(CSUB_LASTSUCCESS, lastSuccessFilter.obj());

  // second or token
  orion::BSONObjBuilder  or2;
  orion::BSONObjBuilder  exists;
  exists.append("$exists", false);
  or2.append(CSUB_LASTSUCCESS, exists.obj());

  o.append(or1.obj());
  o.append(or2.obj());

  condition.append("_id", orion::OID(subId));
  condition.append("$or", o.arr());

  // update
  orion::BSONObjBuilder update;

  orion::BSONObjBuilder lastSuccessB;
  lastSuccessB.append(CSUB_LASTSUCCESS, lastSuccess);
  lastSuccessB.append(CSUB_LASTSUCCESSCODE, statusCode);

  update.append("$set", lastSuccessB.obj());

  if (collectionUpdate(collection, condition.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Internal Error (error updating 'lastSuccess' for a subscription)"));
  }
}



/* ****************************************************************************
*
* mongoSubCountersUpdate - update subscription counters and timestamps in mongo
*
*/
void mongoSubCountersUpdate
(
  const std::string&  tenant,
  const std::string&  subId,
  long long           count,
  long long           lastNotificationTime,
  long long           lastFailure,
  long long           lastSuccess,
  const std::string&  failureReason,
  long long           statusCode
)
{
  std::string  collection = getSubscribeContextCollectionName(tenant);

  if (subId.empty())
  {
    LM_E(("Runtime Error (empty subscription id)"));
    return;
  }

  if (count > 0)
  {
    mongoSubCountersUpdateCount(collection, subId, count);
  }

  if (lastNotificationTime > 0)
  {
    mongoSubCountersUpdateLastNotificationTime(collection, subId, lastNotificationTime);
  }

  if (lastFailure > 0)
  {
    mongoSubCountersUpdateLastFailure(collection, subId, lastFailure, failureReason);
  }

  if (lastSuccess > 0)
  {
    mongoSubCountersUpdateLastSuccess(collection, subId, lastSuccess, statusCode);
  }
}
