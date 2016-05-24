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
#include <string>
#include <regex.h>

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
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/mongoSubCache.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"

using namespace mongo;



/* ****************************************************************************
*
* mongoSubCacheItemInsert - 
*
* RETURN VALUES
*   0:  all OK
*  -1:  Database Error - id-field not found
*  -2:  Out of memory (either returns -2 or exit the entire broker)
*  -3:  No patterned entity found
*  -4:  The vector of notify-conditions is empty
*
*
* Note that the 'count' of the inserted subscription is set to ZERO.
*
*/
int mongoSubCacheItemInsert(const char* tenant, const BSONObj& sub)
{
  LM_W(("In mongoSubCacheItemInsert"));
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
  std::string               renderFormatString = sub.hasField(CSUB_FORMAT)? getFieldF(sub, CSUB_FORMAT).String() : "legacy";  // NGSIv1 JSON is 'default' (for old db-content)
  std::vector<BSONElement>  eVec               = getFieldF(sub, CSUB_ENTITIES).Array();
  std::vector<BSONElement>  attrVec            = getFieldF(sub, CSUB_ATTRS).Array();
  std::vector<BSONElement>  condVec            = getFieldF(sub, CSUB_CONDITIONS).Array();
  RenderFormat              renderFormat       = stringToRenderFormat(renderFormatString);

  cSubP->tenant                = (tenant[0] == 0)? strdup("") : strdup(tenant);
  cSubP->subscriptionId        = strdup(idField.OID().toString().c_str());
  cSubP->servicePath           = strdup(sub.hasField(CSUB_SERVICE_PATH)? getFieldF(sub, CSUB_SERVICE_PATH).String().c_str() : "/");
  cSubP->renderFormat          = renderFormat;
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING)       : -1;
  cSubP->expirationTime        = sub.hasField(CSUB_EXPIRATION)?       getIntOrLongFieldAsLongF(sub, CSUB_EXPIRATION)       : 0;
  cSubP->lastNotificationTime  = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLongF(sub, CSUB_LASTNOTIFICATION) : -1;
  cSubP->status                = sub.hasField(CSUB_STATUS)?           getFieldF(sub, CSUB_STATUS).String().c_str()         : "active";
  cSubP->count                 = 0;
  cSubP->next                  = NULL;


  //
  // 04.2 httpInfo
  //
  // Note that the URL of the notification is stored outside the httpInfo object in mongo
  //
  cSubP->httpInfo.fill(sub);


  //
  // 05. Push Entity-data names to EntityInfo Vector (cSubP->entityInfos)
  //
  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField(CSUB_ENTITY_ID))
    {
      LM_W(("Runtime Error (got a subscription without id)"));
      continue;
    }

    std::string id        = getStringFieldF(entity, ENT_ENTITY_ID);
    std::string isPattern = entity.hasField(CSUB_ENTITY_ISPATTERN)? getStringFieldF(entity, CSUB_ENTITY_ISPATTERN) : "false";
    std::string type      = entity.hasField(CSUB_ENTITY_TYPE)?      getStringFieldF(entity, CSUB_ENTITY_TYPE)      : "";
    EntityInfo* eiP       = new EntityInfo(id, type, isPattern);

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
  for (unsigned int ix = 0; ix < attrVec.size(); ++ix)
  {
    std::string s = attrVec[ix].String();
    cSubP->attributes.push_back(s);
  }



  //
  // 07. Fill in cSubP->notifyConditionVector from condVec
  //
  for (unsigned int ix = 0; ix < condVec.size(); ++ix)
  {
    BSONObj                   condition = condVec[ix].embeddedObject();
    std::string               condType;
    std::vector<BSONElement>  valueVec;

    condType = getStringFieldF(condition, CSUB_CONDITIONS_TYPE);
    if (condType != ON_CHANGE_CONDITION)
    {
      continue;
    }

    NotifyCondition* ncP = new NotifyCondition();
    ncP->type = condType;

    valueVec = getFieldF(condition, CSUB_CONDITIONS_VALUE).Array();
    for (unsigned int vIx = 0; vIx < valueVec.size(); ++vIx)
    {
      std::string condValue;

      condValue = valueVec[vIx].String();
      ncP->condValueList.push_back(condValue);
    }

    cSubP->notifyConditionVector.push_back(ncP);
  }

  if (cSubP->notifyConditionVector.size() == 0)  // Cleanup
  {
    LM_E(("ERROR (empty notifyConditionVector) - cleaning up"));
    subCacheItemDestroy(cSubP);
    delete cSubP;
    return -4;
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
*  -4: Subscription not valid for sub-cache (no patterns)
*  -5: Subscription notvalid for sub-cache (empty notify-condition vector)
*
*
* Note that the 'count' of the inserted subscription is set to ZERO.
*/
int mongoSubCacheItemInsert
(
  const char*         tenant,
  const BSONObj&      sub,
  const char*         subscriptionId,
  const char*         servicePath,
  int                 lastNotificationTime,
  long long           expirationTime,
  const std::string&  status,
  const std::string&  q,
  const std::string&  geometry,
  const std::string&  coords,
  const std::string&  georel,
  StringFilter*       stringFilterP,
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
  // 02. Push Entity-data names to EntityInfo Vector (cSubP->entityInfos)
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

    std::string id        = getStringFieldF(entity, ENT_ENTITY_ID);
    std::string isPattern = entity.hasField(CSUB_ENTITY_ISPATTERN)? getStringFieldF(entity, CSUB_ENTITY_ISPATTERN) : "false";
    std::string type      = entity.hasField(CSUB_ENTITY_TYPE)?      getStringFieldF(entity, CSUB_ENTITY_TYPE)      : "";
    EntityInfo* eiP       = new EntityInfo(id, type, isPattern);

    cSubP->entityIdInfos.push_back(eiP);
  }

  if (cSubP->entityIdInfos.size() == 0)
  {
    subCacheItemDestroy(cSubP);
    delete cSubP;
    return -4;
  }


  //
  // 03. Extract data from mongo sub
  //
  std::vector<BSONElement>  attrVec       = getFieldF(sub, CSUB_ATTRS).Array();
  std::vector<BSONElement>  condVec       = getFieldF(sub, CSUB_CONDITIONS).Array();

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
  cSubP->httpInfo.url          = strdup(sub.hasField(CSUB_REFERENCE)? getFieldF(sub, CSUB_REFERENCE).String().c_str() : "NO REF");  // Mandatory
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING) : -1;
  cSubP->expirationTime        = expirationTime;
  cSubP->lastNotificationTime  = lastNotificationTime;
  cSubP->count                 = 0;
  cSubP->status                = status;
  cSubP->expression.q          = q;
  cSubP->expression.geometry   = geometry;
  cSubP->expression.coords     = coords;
  cSubP->expression.georel     = georel;
  cSubP->next                  = NULL;

  if (stringFilterP != NULL)
  {
    std::string errorString;

    //
    // NOTE
    //   Here, the subscription should have a String Filter but if fill() fails, it won't.
    //   The subscription is already in mongo and hopefully this erroneous situation is fixed 
    //   once the sub-cache is refreshed.
    //
    //   This 'but' should be minimized once the issue 2082 gets implemented.
    //   [ Only reason for fill() to fail (apart from out-of-memory) seems to be an invalid regex ]
    //
    cSubP->expression.stringFilter.fill(stringFilterP, &errorString);
  }

  LM_T(LmtSubCache, ("set lastNotificationTime to %lu for '%s' (from DB)", cSubP->lastNotificationTime, cSubP->subscriptionId));


  //
  // 06. Push attribute names to Attribute Vector (cSubP->attributes)
  //
  for (unsigned int ix = 0; ix < attrVec.size(); ++ix)
  {
    std::string s = attrVec[ix].String();
    cSubP->attributes.push_back(s);
  }



  //
  // 07. Fill in cSubP->notifyConditionVector from condVec
  //
  for (unsigned int ix = 0; ix < condVec.size(); ++ix)
  {
    BSONObj                   condition = condVec[ix].embeddedObject();
    std::string               condType;
    std::vector<BSONElement>  valueVec;

    condType = getStringFieldF(condition, CSUB_CONDITIONS_TYPE);
    if (condType != ON_CHANGE_CONDITION)
    {
      continue;
    }

    NotifyCondition* ncP = new NotifyCondition();
    ncP->type = condType;

    valueVec = getFieldF(condition, CSUB_CONDITIONS_VALUE).Array();
    for (unsigned int vIx = 0; vIx < valueVec.size(); ++vIx)
    {
      std::string condValue;

      condValue = valueVec[vIx].String();
      ncP->condValueList.push_back(condValue);
    }

    cSubP->notifyConditionVector.push_back(ncP);
  }

  if (cSubP->notifyConditionVector.size() == 0)
  {
    subCacheItemDestroy(cSubP);
    delete cSubP;
    return -5;
  }

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
  LM_W(("KZ: In mongoSubCacheRefresh"));
  LM_T(LmtSubCache, ("Refreshing subscription cache for DB '%s'", database.c_str()));

  BSONObj                   query       = BSON("conditions.type" << ON_CHANGE_CONDITION);
  std::string               db          = database;
  std::string               tenant      = tenantFromDb(db);
  std::string               collection  = getSubscribeContextCollectionName(tenant);
  auto_ptr<DBClientCursor>  cursor;
  std::string               errorString;

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (collectionQuery(connection, collection, query, &cursor, &errorString) != true)
  {
    LM_W(("KZ: In mongoSubCacheRefresh: collectionQuery gave false"));
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  int subNo = 0;
  LM_W(("KZ: In mongoSubCacheRefresh: loop over cursor"));
  while (moreSafe(cursor))
  {
    BSONObj      sub;
    std::string  err;

    LM_W(("KZ: In mongoSubCacheRefresh: in loop"));
    if (!nextSafeOrErrorF(cursor, &sub, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      continue;
    }

    LM_W(("KZ: In mongoSubCacheRefresh: calling mongoSubCacheItemInsert"));
    int r = mongoSubCacheItemInsert(tenant.c_str(), sub);

    if (r == 0)
    {
      ++subNo;
    }
  }
  releaseMongoConnection(connection);

  LM_T(LmtSubCache, ("Added %d subscriptions for database '%s'", subNo, database.c_str()));
  LM_W(("KZ: In mongoSubCacheRefresh: FROM"));
}



/* ****************************************************************************
*
* mongoSubCacheUpdate - update subscription in mongo with count and lastNotificationTime
*/
void mongoSubCacheUpdate(const std::string& tenant, const std::string& subId, long long count, long long lastNotificationTime)
{
  std::string  collection  = getSubscribeContextCollectionName(tenant);
  BSONObj      condition;
  BSONObj      update;
  std::string  err;

  if (count != 0)
  {
    // Update count
    condition = BSON("_id"  << OID(subId));
    update    = BSON("$inc" << BSON(CSUB_COUNT << count));

    if (collectionUpdate(collection, condition, update, false, &err) != true)
    {
      LM_E(("Internal Error (error updating 'count' for a subscription)"));
    }
  }

  if (lastNotificationTime != 0)
  {
    // Update lastNotificationTime    
    condition = BSON("_id" << OID(subId) << "$or" << BSON_ARRAY(
                       BSON(CSUB_LASTNOTIFICATION << BSON("$lt" << lastNotificationTime)) <<
                       BSON(CSUB_LASTNOTIFICATION << BSON("$exists" << false)))
                    );
    update    = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << lastNotificationTime));

    if (collectionUpdate(collection, condition, update, false, &err) != true)
    {
      LM_E(("Internal Error (error updating 'lastNotification' for a subscription)"));
    }
  }
}
