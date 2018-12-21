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

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "common/string.h"
#include "common/statistics.h"
#include "common/RenderFormat.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/StringFilter.h"
#include "cache/subCache.h"

#ifdef ORIONLD
#include "orionld/common/OrionldConnection.h"                  // orionldState
#endif

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoSubCache.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONObj;
using mongo::BSONElement;
using mongo::DBClientCursor;
using mongo::DBClientBase;
using mongo::OID;



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
int mongoSubCacheItemInsert(const char* tenant, const BSONObj& sub)
{
  //
  // 01. Check validity of subP parameter
  //
  BSONElement  idField = getFieldF(sub, "_id");

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
  std::string    renderFormatString = sub.hasField(CSUB_FORMAT)? getStringFieldF(sub, CSUB_FORMAT) : "legacy";
  RenderFormat   renderFormat       = stringToRenderFormat(renderFormatString);

  cSubP->tenant = (tenant[0] == 0)? strdup("") : strdup(tenant);

#ifdef ORIONLD
  //
  // FIXME: Check whether idField is an OID before calling OID
  //        Should check whether the subscription was created with NGSI-LD or not
  //        I can always check idField.toString().c_str() ...
  //
  if (orionldState.apiVersion == NGSI_LD_V1)
    cSubP->subscriptionId        = strdup(idField.toString().c_str());
  else
    cSubP->subscriptionId        = strdup(idField.OID().toString().c_str());
#else
  cSubP->subscriptionId        = strdup(idField.OID().toString().c_str());
#endif

  cSubP->servicePath           = strdup(sub.hasField(CSUB_SERVICE_PATH)? getStringFieldF(sub, CSUB_SERVICE_PATH).c_str() : "/");
  cSubP->renderFormat          = renderFormat;
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING)       : -1;
  cSubP->expirationTime        = sub.hasField(CSUB_EXPIRATION)?       getIntOrLongFieldAsLongF(sub, CSUB_EXPIRATION)       : 0;
  cSubP->lastNotificationTime  = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongF(sub, CSUB_LASTNOTIFICATION) : -1;
  cSubP->status                = sub.hasField(CSUB_STATUS)?           getStringFieldF(sub, CSUB_STATUS).c_str()            : "active";
  cSubP->blacklist             = sub.hasField(CSUB_BLACKLIST)?        getBoolFieldF(sub, CSUB_BLACKLIST)                   : false;
  cSubP->lastFailure           = sub.hasField(CSUB_LASTFAILURE)?      getIntOrLongFieldAsLongF(sub, CSUB_LASTFAILURE)      : -1;
  cSubP->lastSuccess           = sub.hasField(CSUB_LASTSUCCESS)?      getIntOrLongFieldAsLongF(sub, CSUB_LASTSUCCESS)      : -1;
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
    BSONObj expression = getObjectFieldF(sub, CSUB_EXPR);

    if (expression.hasField(CSUB_EXPR_Q))
    {
      std::string errorString;

      cSubP->expression.q = getStringFieldF(expression, CSUB_EXPR_Q);
      if (cSubP->expression.q != "")
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

      cSubP->expression.mq = getStringFieldF(expression, CSUB_EXPR_MQ);
      if (cSubP->expression.mq != "")
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
      cSubP->expression.geometry = getStringFieldF(expression, CSUB_EXPR_GEOM);
    }

    if (expression.hasField(CSUB_EXPR_COORDS))
    {
      cSubP->expression.coords = getStringFieldF(expression, CSUB_EXPR_COORDS);
    }

    if (expression.hasField(CSUB_EXPR_GEOREL))
    {
      cSubP->expression.georel = getStringFieldF(expression, CSUB_EXPR_GEOREL);
    }
  }


  //
  // 05. Push Entity-data names to EntityInfo Vector (cSubP->entityInfos)
  //
  std::vector<BSONElement>  eVec = getFieldF(sub, CSUB_ENTITIES).Array();
  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField(CSUB_ENTITY_ID))
    {
      LM_W(("Runtime Error (got a subscription without id)"));
      continue;
    }

    std::string id            = getStringFieldF(entity, ENT_ENTITY_ID);
    std::string isPattern     = entity.hasField(CSUB_ENTITY_ISPATTERN)? getStringFieldF(entity, CSUB_ENTITY_ISPATTERN) : "false";
    std::string type          = entity.hasField(CSUB_ENTITY_TYPE)?      getStringFieldF(entity, CSUB_ENTITY_TYPE)      : "";
    bool        isTypePattern = entity.hasField(CSUB_ENTITY_ISTYPEPATTERN)? getBoolFieldF(entity, CSUB_ENTITY_ISTYPEPATTERN) : false;
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
  setStringVectorF(sub, CSUB_ATTRS, &(cSubP->attributes));


  //
  // 07. Fill in cSubP->notifyConditionV from condVec
  //
  setStringVectorF(sub, CSUB_CONDITIONS, &(cSubP->notifyConditionV));


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
  const char*         tenant,
  const BSONObj&      sub,
  const char*         subscriptionId,
  const char*         servicePath,
  int                 lastNotificationTime,
  int                 lastFailure,
  int                 lastSuccess,
  long long           expirationTime,
  const std::string&  status,
  const std::string&  q,
  const std::string&  mq,
  const std::string&  geometry,
  const std::string&  coords,
  const std::string&  georel,
  StringFilter*       stringFilterP,
  StringFilter*       mdStringFilterP,
  RenderFormat        renderFormat
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
  std::vector<BSONElement>  eVec = getFieldF(sub, CSUB_ENTITIES).Array();

  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField(CSUB_ENTITY_ID))
    {
      LM_W(("Runtime Error (got a subscription without id)"));
      continue;
    }

    std::string id            = getStringFieldF(entity, ENT_ENTITY_ID);
    std::string isPattern     = entity.hasField(CSUB_ENTITY_ISPATTERN)? getStringFieldF(entity, CSUB_ENTITY_ISPATTERN) : "false";
    std::string type          = entity.hasField(CSUB_ENTITY_TYPE)?      getStringFieldF(entity, CSUB_ENTITY_TYPE)      : "";
    bool        isTypePattern = entity.hasField(CSUB_ENTITY_ISTYPEPATTERN)? getBoolFieldF(entity, CSUB_ENTITY_ISTYPEPATTERN) : false;
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
    lastNotificationTime = getIntOrLongFieldAsLongF(sub, CSUB_LASTNOTIFICATION);
  }

  cSubP->tenant                = (tenant[0] == 0)? NULL : strdup(tenant);
  cSubP->subscriptionId        = strdup(subscriptionId);
  cSubP->servicePath           = strdup(servicePath);
  cSubP->renderFormat          = renderFormat;
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING) : -1;
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
  cSubP->blacklist             = sub.hasField(CSUB_BLACKLIST)? getBoolFieldF(sub, CSUB_BLACKLIST) : false;

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
  setStringVectorF(sub, CSUB_ATTRS, &(cSubP->attributes));

  //
  // 07. Push metadata names to Metadata Vector (cSubP->metadatas)
  //
  if (sub.hasField(CSUB_METADATA))
  {
    setStringVectorF(sub, CSUB_METADATA, &(cSubP->metadata));
  }

  //
  // 08. Fill in cSubP->notifyConditionV from condVec
  //
  setStringVectorF(sub, CSUB_CONDITIONS, &(cSubP->notifyConditionV));

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

  BSONObj                        query;      // empty query (all subscriptions)
  std::string                    db          = database;
  std::string                    tenant      = tenantFromDb(db);
  std::string                    collection  = getSubscribeContextCollectionName(tenant);
  std::auto_ptr<DBClientCursor>  cursor;
  std::string                    errorString;

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (collectionQuery(connection, collection, query, &cursor, &errorString) != true)
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  int subNo = 0;
  while (moreSafe(cursor))
  {
    BSONObj      sub;
    std::string  err;

    if (!nextSafeOrErrorF(cursor, &sub, &err))
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
  releaseMongoConnection(connection);

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
  BSONObj      condition;
  BSONObj      update;
  std::string  err;

  condition = BSON("_id"  << OID(subId));
  update    = BSON("$inc" << BSON(CSUB_COUNT << count));

  if (collectionUpdate(collection, condition, update, false, &err) != true)
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
  BSONObj      condition;
  BSONObj      update;
  std::string  err;

  condition = BSON("_id" << OID(subId) << "$or" << BSON_ARRAY(
                     BSON(CSUB_LASTNOTIFICATION << BSON("$lt" << lastNotificationTime)) <<
                     BSON(CSUB_LASTNOTIFICATION << BSON("$exists" << false))));
  update    = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << lastNotificationTime));

  if (collectionUpdate(collection, condition, update, false, &err) != true)
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
  long long           lastFailure
)
{
  BSONObj      condition;
  BSONObj      update;
  std::string  err;

  condition = BSON("_id" << OID(subId) << "$or" << BSON_ARRAY(
                     BSON(CSUB_LASTFAILURE << BSON("$lt" << lastFailure)) <<
                     BSON(CSUB_LASTFAILURE << BSON("$exists" << false))));
  update    = BSON("$set" << BSON(CSUB_LASTFAILURE << lastFailure));

  if (collectionUpdate(collection, condition, update, false, &err) != true)
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
  long long           lastSuccess
)
{
  BSONObj      condition;
  BSONObj      update;
  std::string  err;

  condition = BSON("_id" << OID(subId) << "$or" << BSON_ARRAY(
                     BSON(CSUB_LASTSUCCESS << BSON("$lt" << lastSuccess)) <<
                     BSON(CSUB_LASTSUCCESS << BSON("$exists" << false))));
  update    = BSON("$set" << BSON(CSUB_LASTSUCCESS << lastSuccess));

  if (collectionUpdate(collection, condition, update, false, &err) != true)
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
  const std::string& tenant,
  const std::string& subId,
  long long          count,
  long long          lastNotificationTime,
  long long          lastFailure,
  long long          lastSuccess
)
{
  std::string  collection = getSubscribeContextCollectionName(tenant);

  if (subId == "")
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
    mongoSubCountersUpdateLastFailure(collection, subId, lastFailure);
  }

  if (lastSuccess > 0)
  {
    mongoSubCountersUpdateLastSuccess(collection, subId, lastSuccess);
  }
}
