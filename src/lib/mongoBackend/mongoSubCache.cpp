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

#include "mongoBackend/MongoGlobal.h"
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
* Note that the 'count' and 'failsCounter' of the inserted subscription are set to ZERO.
*
*/
int mongoSubCacheItemInsert(const char* tenant, const orion::BSONObj& sub)
{
  //
  // 01. Check validity of subP parameter
  //
  orion::BSONElement  idField = getFieldF(sub, "_id");

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

  cSubP->tenant                = (tenant[0] == 0)? strdup("") : strdup(tenant);
  cSubP->subscriptionId        = strdup(idField.OID().c_str());
  cSubP->servicePath           = strdup(sub.hasField(CSUB_SERVICE_PATH)? getStringFieldF(sub, CSUB_SERVICE_PATH).c_str() : "/");
  cSubP->renderFormat          = renderFormat;
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING)       : -1;
  cSubP->maxFailsLimit         = sub.hasField(CSUB_MAXFAILSLIMIT)?    getIntOrLongFieldAsLongF(sub, CSUB_MAXFAILSLIMIT)    : -1;
  cSubP->expirationTime        = sub.hasField(CSUB_EXPIRATION)?       getIntOrLongFieldAsLongF(sub, CSUB_EXPIRATION)       :  0;
  cSubP->lastNotificationTime  = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongF(sub, CSUB_LASTNOTIFICATION) : -1;
  cSubP->status                = sub.hasField(CSUB_STATUS)?           getStringFieldF(sub, CSUB_STATUS)                    : "active";
  cSubP->statusLastChange      = sub.hasField(CSUB_STATUS_LAST_CHANGE)? getNumberFieldF(sub, CSUB_STATUS_LAST_CHANGE)      : -1;
  cSubP->blacklist             = sub.hasField(CSUB_BLACKLIST)?        getBoolFieldF(sub, CSUB_BLACKLIST)                   : false;
  cSubP->lastFailure           = sub.hasField(CSUB_LASTFAILURE)?      getIntOrLongFieldAsLongF(sub, CSUB_LASTFAILURE)      : -1;
  cSubP->lastSuccess           = sub.hasField(CSUB_LASTSUCCESS)?      getIntOrLongFieldAsLongF(sub, CSUB_LASTSUCCESS)      : -1;
  cSubP->lastFailureReason     = sub.hasField(CSUB_LASTFAILUREASON)?  getStringFieldF(sub, CSUB_LASTFAILUREASON)           : "";
  cSubP->lastSuccessCode       = sub.hasField(CSUB_LASTSUCCESSCODE)?  getIntOrLongFieldAsLongF(sub, CSUB_LASTSUCCESSCODE)  : -1;
  cSubP->count                 = 0;
  cSubP->failsCounter          = 0;
  cSubP->onlyChanged           = sub.hasField(CSUB_ONLYCHANGED)?      getBoolFieldF(sub, CSUB_ONLYCHANGED)                 : false;
  cSubP->covered               = sub.hasField(CSUB_COVERED)?          getBoolFieldF(sub, CSUB_COVERED)                     : false;
  cSubP->notifyOnMetadataChange = sub.hasField(CSUB_NOTIFYONMETADATACHANGE)? getBoolFieldF(sub, CSUB_NOTIFYONMETADATACHANGE) : true;
  cSubP->next                  = NULL;

  // getIntOrLongFieldAsLong() may return -1 if something goes wrong, so we add a guard to set 0 in this case
  cSubP->failsCounterFromDb = sub.hasField(CSUB_FAILSCOUNTER)? getIntOrLongFieldAsLongF(sub, CSUB_FAILSCOUNTER) : 0;
  if (cSubP->failsCounterFromDb < 0)
  {
    cSubP->failsCounterFromDb = 0;
  }

  // set to valid at refresh time (to be invalidated if some sucess occurs before the next cache refresh cycle)
  cSubP->failsCounterFromDbValid = true;

  //
  // 04.2 httpInfo & mqttInfo
  //
  // Note that the URL of the notification is stored outside the httpInfo object in mongo
  //
  if (sub.hasField(CSUB_MQTTTOPIC))
  {
    cSubP->mqttInfo.fill(sub);
  }
  else
  {
    cSubP->httpInfo.fill(sub);
  }


  //
  // 04.3 expression
  //
  if (sub.hasField(CSUB_EXPR))
  {
    orion::BSONObj expression = getObjectFieldF(sub, CSUB_EXPR);

    if (expression.hasField(CSUB_EXPR_Q))
    {
      std::string errorString;

      cSubP->expression.q = getStringFieldF(expression, CSUB_EXPR_Q);
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

      cSubP->expression.mq = getStringFieldF(expression, CSUB_EXPR_MQ);
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
  std::vector<orion::BSONElement>  eVec = getFieldF(sub, CSUB_ENTITIES).Array();
  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    orion::BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField(CSUB_ENTITY_ID))
    {
      LM_E(("Runtime Error (got a subscription without id)"));
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

  //
  // 09. Fill in cSubP->subAltTypeV from alteration types
  //
  if (sub.hasField(CSUB_ALTTYPES))
  {
    std::vector<std::string> altTypeStrings;
    setStringVectorF(sub, CSUB_ALTTYPES, &altTypeStrings);

    for (unsigned int ix = 0; ix < altTypeStrings.size(); ix++)
    {
      ngsiv2::SubAltType altType = parseAlterationType(altTypeStrings[ix]);
      if (altType == ngsiv2::SubAltType::Unknown)
      {
        LM_E(("Runtime Error (unknown alterationType found in database)"));
      }
      else
      {
        cSubP->subAltTypeV.push_back(altType);
      }
    }
  }

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
* Note that the 'count' and 'failsCounter" of the inserted subscription are set to ZERO.
* This is because the sub cache only counts the increments in these accumulating counters,
* so that other CBs, operating on the same DB will not overwrite the value of these accumulators
*/
int mongoSubCacheItemInsert
(
  const char*            tenant,
  const orion::BSONObj&  sub,
  const char*            subscriptionId,
  const char*            servicePath,
  long long              lastNotificationTime,
  long long              lastFailure,
  const std::string&     lastFailureReason,
  long long              lastSuccess,
  long long              lastSuccessCode,
  long long              count,
  long long              failsCounter,
  long long              failsCounterFromDb,
  bool                   failsCounterFromDbValid,
  long long              expirationTime,
  const std::string&     status,
  double                 statusLastChange,
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
  std::vector<orion::BSONElement>  eVec = getFieldF(sub, CSUB_ENTITIES).Array();

  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    orion::BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField(CSUB_ENTITY_ID))
    {
      LM_E(("Runtime Error (got a subscription without id)"));
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


  // 04. Extract data from subP
  //
  // NOTE: NGSIv1 JSON is 'default' (for old db-content)
  //
  cSubP->tenant                = (tenant[0] == 0)? NULL : strdup(tenant);
  cSubP->subscriptionId        = strdup(subscriptionId);
  cSubP->servicePath           = strdup(servicePath);
  cSubP->renderFormat          = renderFormat;
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)? getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING) : -1;
  cSubP->maxFailsLimit         = sub.hasField(CSUB_MAXFAILSLIMIT)? getIntOrLongFieldAsLongF(sub, CSUB_MAXFAILSLIMIT) : -1;
  cSubP->expirationTime        = expirationTime;
  cSubP->expression.q          = q;
  cSubP->expression.mq         = mq;
  cSubP->expression.geometry   = geometry;
  cSubP->expression.coords     = coords;
  cSubP->expression.georel     = georel;
  cSubP->next                  = NULL;
  cSubP->blacklist             = sub.hasField(CSUB_BLACKLIST)? getBoolFieldF(sub, CSUB_BLACKLIST) : false;
  cSubP->covered               = sub.hasField(CSUB_COVERED)? getBoolFieldF(sub, CSUB_COVERED) : false;
  cSubP->notifyOnMetadataChange = sub.hasField(CSUB_NOTIFYONMETADATACHANGE)? getBoolFieldF(sub, CSUB_NOTIFYONMETADATACHANGE) : true;

  cSubP->lastNotificationTime  = lastNotificationTime;
  cSubP->lastFailure           = lastFailure;
  cSubP->lastFailureReason     = lastFailureReason;
  cSubP->lastSuccess           = lastSuccess;
  cSubP->lastSuccessCode       = lastSuccessCode;
  cSubP->count                 = count;
  cSubP->failsCounter          = failsCounter;
  cSubP->failsCounterFromDb    = failsCounterFromDb;
  cSubP->failsCounterFromDbValid  = failsCounterFromDbValid;
  cSubP->status                = status;
  cSubP->statusLastChange      = statusLastChange;

  //
  // httpInfo & mqttInfo
  //
  // Note that the URL of the notification is stored outside the httpInfo object in mongo
  //
  if (sub.hasField(CSUB_MQTTTOPIC))
  {
    cSubP->mqttInfo.fill(sub);
  }
  else
  {
    cSubP->httpInfo.fill(sub);
  }

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

  //
  // 09. Fill in cSubP->subAltTypeV from alterationTypes
  //
  if (sub.hasField(CSUB_ALTTYPES))
  {
    std::vector<std::string> altTypeStrings;
    setStringVectorF(sub, CSUB_ALTTYPES, &altTypeStrings);

    for (unsigned int ix = 0; ix < altTypeStrings.size(); ix++)
    {
      ngsiv2::SubAltType altType = parseAlterationType(altTypeStrings[ix]);
      if (altType == ngsiv2::SubAltType::Unknown)
      {
        LM_E(("Runtime Error (unknown alterationType found in database)"));
      }
      else
      {
        cSubP->subAltTypeV.push_back(altType);
      }
    }
  }

  subCacheItemInsert(cSubP);

  return 0;
}



/* ****************************************************************************
*
* mongoSubCacheRefresh -
*
* Cache has been emptied before calling this function
*
* 1. Lookup all subscriptions in the database
* 2. Insert them again in the cache (with fresh data from database)
*
*/
void mongoSubCacheRefresh(const std::string& database)
{
  LM_T(LmtSubCache, ("Refreshing subscription cache for DB '%s'", database.c_str()));

  orion::BSONObj   query;      // empty query (all subscriptions)
  std::string      tenant      = tenantFromDb(database);
  orion::DBCursor  cursor;
  std::string      errorString;

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();
  if (orion::collectionQuery(connection, database, COL_CSUBS, query, &cursor, &errorString) != true)
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  int subNo = 0;
  orion::BSONObj  sub;
  while (cursor.next(&sub))
  {
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
* mongoSubCountersUpdateFailsAndStatus -
*/
static void mongoSubCountersUpdateFailsAndStatus
(
  const std::string&  db,
  const std::string&  collection,
  const std::string&  subId,
  long long           fails,
  const std::string&  status,
  double              statusLastChange
)
{
  orion::BSONObjBuilder  condition;
  orion::BSONObjBuilder  update;
  orion::BSONObjBuilder  incB;
  orion::BSONObjBuilder  setB;

  std::string  err;

  condition.append("_id", orion::OID(subId));

  if (fails > 0)
  {
    incB.append(CSUB_FAILSCOUNTER, fails);
  }
  else if (noCache)
  {
    // no fails mean notification ok, thus reseting the counter
    // in noCache case, this is always done. In cache case, it will be done
    // at cache refresh time (check mongoSubUpdateOnCacheSync())
    setB.append(CSUB_FAILSCOUNTER, 0);
  }

  if (!status.empty())
  {
    setB.append(CSUB_STATUS, status);
    setB.append(CSUB_STATUS_LAST_CHANGE, statusLastChange);
  }

  if ((incB.nFields() == 0) && (setB.nFields() == 0))
  {
    // no info to update
    return;
  }

  if (incB.nFields() > 0)
  {
    update.append("$inc", incB.obj());
  }
  if (setB.nFields() > 0)
  {
    update.append("$set", setB.obj());
  }

  if (collectionUpdate(db, collection, condition.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Runtime Error (error updating 'count', 'failsCounter', 'status' and/or 'statusLastChange' for a subscription)"));
  }
}



/* ****************************************************************************
*
* mongoSubCountersUpdateLastNotificationTime -
*/
static void mongoSubCountersUpdateLastNotificationTime
(
  const std::string&  db,
  const std::string&  collection,
  const std::string&  subId,
  long long           lastNotificationTime
)
{
  std::string  err;

  // Note we cannot apply this simplification for lastFailure or lastSucess due to
  // in that case there is a side field (lastFailureReason or lastSuccess code) that
  // needs to be updated at the same time and $max doesn't help in that case

  orion::BSONObjBuilder id;
  id.append("_id", orion::OID(subId));

  orion::BSONObjBuilder update;
  orion::BSONObjBuilder maxB;
  maxB.append(CSUB_LASTNOTIFICATION, lastNotificationTime);
  update.append("$max", maxB.obj());

  if (orion::collectionUpdate(db, collection, id.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Runtime Error (error updating 'lastNotification' for a subscription)"));
  }
}



/* ****************************************************************************
*
* mongoSubCountersUpdateLastFailure -
*/
static void mongoSubCountersUpdateLastFailure
(
  const std::string&  db,
  const std::string&  collection,
  const std::string&  subId,
  long long           lastFailure,
  const std::string&  failureReason
)
{
  std::string  err;

  // FIXME #3774: previously this part was based in streamming instead of append()

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

  if (orion::collectionUpdate(db, collection, condition.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Runtime Error (error updating 'lastFailure' for a subscription)"));
  }
}



/* ****************************************************************************
*
* mongoSubCountersUpdateLastSuccess -
*/
static void mongoSubCountersUpdateLastSuccess
(
  const std::string&  db,
  const std::string&  collection,
  const std::string&  subId,
  long long           lastSuccess,
  long long           statusCode
)
{
  std::string  err;

  // FIXME #3774: previously this part was based in streamming instead of append()

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

  if (collectionUpdate(db, collection, condition.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Runtime Error (error updating 'lastSuccess' for a subscription)"));
  }
}



/* ****************************************************************************
*
* mongoSubUpdateOnNotif - update subscription doc in mongo due to a notification
*
* Used in notification logic
*
* Although we are updating basically the same things (lastNotificationTime, lastSuccess, etc.),
* we cannot use mongoSubUpdateOnCacheSync(). Note that in mongoSubUpdateOnCacheSync()
* we start from a reference status in DB so we can decide what to update in a
* single shot. However, in the notification case we don't know the status of the
* DB so we need updates with a query part adapted to a possibly newer data in DB
* (e.g. we use $max for lastNotificationTime).
*/
void mongoSubUpdateOnNotif
(
  const std::string&  tenant,
  const std::string&  subId,
  long long           failsCounter,
  long long           lastNotificationTime,
  long long           lastFailure,
  long long           lastSuccess,
  const std::string&  failureReason,
  long long           statusCode,
  const std::string&  status,
  double              statusLastChange
)
{
  if (subId.empty())
  {
    LM_E(("Runtime Error (empty subscription id)"));
    return;
  }

  std::string db = composeDatabaseName(tenant);

  mongoSubCountersUpdateFailsAndStatus(db, COL_CSUBS, subId, failsCounter, status, statusLastChange);

  if (lastNotificationTime > 0)
  {
    mongoSubCountersUpdateLastNotificationTime(db, COL_CSUBS, subId, lastNotificationTime);
  }

  if (lastFailure > 0)
  {
    mongoSubCountersUpdateLastFailure(db, COL_CSUBS, subId, lastFailure, failureReason);
  }

  if (lastSuccess > 0)
  {
    mongoSubCountersUpdateLastSuccess(db, COL_CSUBS, subId, lastSuccess, statusCode);
  }
}


/* ****************************************************************************
*
* mongoSubUpdateOnCacheSync -
*
* Used in cache sync logic
*/
void mongoSubUpdateOnCacheSync
(
  const std::string&  tenant,
  const std::string&  subId,
  long long           count,
  long long           failsCounter,
  int64_t*            lastNotificationTimeP,
  int64_t*            lastFailureP,
  int64_t*            lastSuccessP,
  std::string*        failureReasonP,
  int64_t*            statusCodeP,
  std::string*        statusP,
  double*             statusLastChangeP
)
{
  orion::BSONObjBuilder  condition;
  orion::BSONObjBuilder  update;
  orion::BSONObjBuilder  setB;
  orion::BSONObjBuilder  incB;

  if (count > 0)
  {
    incB.append(CSUB_COUNT, count);
  }
  if (failsCounter > 0)
  {
    incB.append(CSUB_FAILSCOUNTER, failsCounter);
  }
  else if (count > 0)
  {
    // no fails mean notification ok, thus reseting the counter. The count > 0 check is needed to
    // ensure that at least one notification has been sent in since last cache refresh
    // see cases/3541_subscription_max_fails_limit/failsCounter_keeps_after_cache_refresh_cycles.test
    setB.append(CSUB_FAILSCOUNTER, 0);
  }

  if ((lastNotificationTimeP != NULL) && (*lastNotificationTimeP > 0))
  {
    setB.append(CSUB_LASTNOTIFICATION, (long long) *lastNotificationTimeP);
  }
  if ((lastFailureP != NULL) && (*lastFailureP > 0))
  {
    setB.append(CSUB_LASTFAILURE, (long long) *lastFailureP);
  }
  if ((lastSuccessP != NULL) && (*lastSuccessP > 0))
  {
    setB.append(CSUB_LASTSUCCESS, (long long) *lastSuccessP);
  }
  if (failureReasonP != NULL)
  {
    setB.append(CSUB_LASTFAILUREASON, *failureReasonP);
  }
  if (statusCodeP != NULL)
  {
    setB.append(CSUB_LASTSUCCESSCODE, (long long) *statusCodeP);
  }
  if (statusP != NULL)
  {
    setB.append(CSUB_STATUS, *statusP);
  }
  if (statusLastChangeP != NULL)
  {
    setB.append(CSUB_STATUS_LAST_CHANGE, *statusLastChangeP);
  }

  if ((incB.nFields() == 0) && (setB.nFields() == 0))
  {
    // Nothing to update, return
    return;
  }

  if (incB.nFields() > 0)
  {
    update.append("$inc", incB.obj());
  }
  if (setB.nFields() > 0)
  {
    update.append("$set", setB.obj());
  }

  condition.append("_id", orion::OID(subId));

  std::string  err;
  if (collectionUpdate(composeDatabaseName(tenant), COL_CSUBS, condition.obj(), update.obj(), false, &err) != true)
  {
    LM_E(("Runtime Error (error updating subs during cache sync: %s)", err.c_str()));
  }
}

