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

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubCache.h"

#include "ngsi/NotifyConditionVector.h"
#include "ngsi10/SubscribeContextRequest.h"

using namespace mongo;
using std::auto_ptr;



/* ****************************************************************************
*
* EntityInfo::EntityInfo - 
*/
EntityInfo::EntityInfo(const std::string& idPattern, const std::string& _entityType)
{
  regcomp(&entityIdPattern, idPattern.c_str(), 0);

  entityType               = _entityType;
  entityIdPatternToBeFreed = true;
}



/* ****************************************************************************
*
* EntityInfo::match - 
*/
bool EntityInfo::match
(
  const std::string&  id,
  const std::string&  type
)
{
  //
  // If type non-empty - perfect match is mandatory
  // If type is empty - always matches
  //
  if ((type != "") && (entityType != type) && (entityType != ""))
  {
    return false;
  }

  // REGEX-comparison this->entityIdPattern VS id
  return regexec(&entityIdPattern, id.c_str(), 0, NULL, 0) == 0;
}



/* ****************************************************************************
*
* EntityInfo::release - 
*/
void EntityInfo::release(void)
{
  if (entityIdPatternToBeFreed)
  {
    regfree(&entityIdPattern);
    entityIdPatternToBeFreed = false;
  }
}



/* ****************************************************************************
*
* EntityInfo::present - 
*/
void EntityInfo::present(const std::string& prefix)
{
  LM_F(("%sid:    regex describing EntityId::id (the original string is not saved)", prefix.c_str()));
  LM_F(("%stype:  %s", prefix.c_str(), entityType.c_str()));
}



/* ****************************************************************************
*
* MongoSubCache - 
*/
typedef struct MongoSubCache
{
  CachedSubscription* head;
  CachedSubscription* tail;
  int                 items;

  // Statistics counters
  int                 noOfRefreshes;
  int                 noOfInserts;
  int                 noOfRemoves;
} MongoSubCache;



/* ****************************************************************************
*
* mongoSubCache - 
*/
MongoSubCache  mongoSubCache = { NULL, NULL, 0 };



/* ****************************************************************************
*
* mongoSubCacheInit - 
*/
void mongoSubCacheInit(void)
{
  LM_T(LmtMongoSubCache, ("Initializing subscription cache"));

  mongoSubCache.head   = NULL;
  mongoSubCache.tail   = NULL;
  mongoSubCache.items  = 0;

  mongoSubCacheStatisticsReset();
}



/* ****************************************************************************
*
* mongoSubCacheItemInsert - 
*/
static void mongoSubCacheItemInsert(CachedSubscription* cSubP)
{
  //
  // First of all, insertion is done at the end of the list, so, 
  // cSubP->next must ALWAYS be zero at insertion time
  //
  cSubP->next = NULL;

  LM_T(LmtMongoSubCache, ("inserting sub '%s', lastNotificationTime: %lu", cSubP->subscriptionId, cSubP->lastNotificationTime));

  // First insertion?
  if ((mongoSubCache.head == NULL) && (mongoSubCache.tail == NULL))
  {
    mongoSubCache.head   = cSubP;
    mongoSubCache.tail   = cSubP;
    mongoSubCache.items  = 1;
    return;
  }

  
  mongoSubCache.tail->next  = cSubP;
  mongoSubCache.items      += 1;
  mongoSubCache.tail        = cSubP;
}



/* ****************************************************************************
*
* attributeMatch - 
*/
static bool attributeMatch(CachedSubscription* cSubP, const char* attr)
{
  for (unsigned int ncvIx = 0; ncvIx < cSubP->notifyConditionVector.size(); ++ncvIx)
  {
    NotifyCondition* ncP = cSubP->notifyConditionVector[ncvIx];

    for (unsigned int cvIx = 0; cvIx < ncP->condValueList.size(); ++cvIx)
    {
      if (ncP->condValueList[cvIx] == attr)
      {
        return true;
      }
    }
  }

  return false;
}



/* ****************************************************************************
*
* servicePathMatch - 
*/
static bool servicePathMatch(CachedSubscription* cSubP, char* servicePath)
{
  const char*  spath;

  // Special case. "/#" matches EVERYTHING
  if ((strlen(servicePath) == 2) && (servicePath[0] == '/') && (servicePath[1] == '#') && (servicePath[2] == 0))
  {
    return true;
  }

  //
  // Empty service path?
  //
  if ((strlen(servicePath) == 0) && (strlen(cSubP->servicePath) == 0))
  {
    return true;
  }

  //
  // Default service path for a subscription is "/".
  // So, if empty, set it to "/"
  //
  if (servicePath[0] == 0)
  {
    servicePath = (char*) "/";
  }

  spath = cSubP->servicePath;

  //
  // No wildcard - exact match
  //
  if (spath[strlen(spath) - 1] != '#')
  {
    return (strcmp(servicePath, cSubP->servicePath) == 0);
  }


  //
  // Wildcard - remove '#' and compare to the length of _servicePath.
  //            If equal upto the length of _servicePath, then it is a match
  //
  // Actually, there is more than one way to match here:
  // If we have a servicePath of the subscription as 
  // "/a/b/#", then the following service-paths must match:
  //   1. /a/b
  //   2. /a/b/ AND /a/b/.+  (any path below /a/b/)
  //
  // What should NOT match is "/a/b2".
  //
  unsigned int len = strlen(spath) - 2;

  // 1. /a/b - (removing 2 chars from the cache-subscription removes "/#"

  if ((spath[len] == '/') && (strlen(servicePath) == len) && (strncmp(spath, servicePath, len) == 0))
  {
    return true;
  }

  // 2. /a/b/.+
  len = strlen(spath) - 1;
  if (strncmp(spath, servicePath, len) == 0)
  {
    return true;
  }
  
  return false;
}



/* ****************************************************************************
*
* subMatch - 
*/
static bool subMatch
(
  CachedSubscription*  cSubP,
  const char*          tenant,
  const char*          servicePath,
  const char*          entityId,
  const char*          entityType,
  const char*          attr
)
{
  if ((cSubP->tenant == NULL) || (tenant == NULL) || (cSubP->tenant[0] == 0) || (tenant[0] == 0))
  {
    if ((cSubP->tenant != NULL) && (cSubP->tenant[0] != 0))
    {
      return false;
    }

    if ((tenant != NULL) && (tenant[0] != 0))
    {
      return false;
    }
  }
  else if (strcmp(cSubP->tenant, tenant) != 0)
  {
    return false;
  }

  if (servicePathMatch(cSubP, (char*) servicePath) == false)
  {
    return false;
  }


  //
  // If ONCHANGE and one of the attribute names in the scope vector
  // of the subscription has the same name as the incoming attribute. there is a match.
  //
  if (!attributeMatch(cSubP, attr))
  {
    return false;
  }

  for (unsigned int ix = 0; ix < cSubP->entityIdInfos.size(); ++ix)
  {
    EntityInfo* eiP = cSubP->entityIdInfos[ix];

    if (eiP->match(entityId, entityType))
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* mongoSubCacheMatch - 
*/
void mongoSubCacheMatch
(
  const char*                        tenant,
  const char*                        servicePath,
  const char*                        entityId,
  const char*                        entityType,
  const char*                        attr,
  std::vector<CachedSubscription*>*  subVecP
)
{
  CachedSubscription* cSubP = mongoSubCache.head;

  while (cSubP != NULL)
  {
    if (subMatch(cSubP, tenant, servicePath, entityId, entityType, attr))
    {
      subVecP->push_back(cSubP);
      LM_T(LmtMongoSubCache, ("added subscription '%s': lastNotificationTime: %lu", cSubP->subscriptionId, cSubP->lastNotificationTime));
    }

    cSubP = cSubP->next;
  }
}



/* ****************************************************************************
*
* cachedSubscriptionDestroy - 
*/
static void cachedSubscriptionDestroy(CachedSubscription* cSubP)
{
  if (cSubP->tenant != NULL)
  {
    free(cSubP->tenant);
    cSubP->tenant = NULL;
  }

  if (cSubP->servicePath != NULL)
  {
    free(cSubP->servicePath);
    cSubP->servicePath = NULL;
  }

  if (cSubP->subscriptionId != NULL)
  {
    free(cSubP->subscriptionId);
    cSubP->subscriptionId = NULL;
  }

  if (cSubP->reference != NULL)
  {
    free(cSubP->reference);
    cSubP->reference = NULL;
  }

  for (unsigned int ix = 0; ix < cSubP->entityIdInfos.size(); ++ix)
  {
    cSubP->entityIdInfos[ix]->release();
    delete cSubP->entityIdInfos[ix];
  }
  cSubP->entityIdInfos.clear();

  while (cSubP->attributes.size() > 0)
  {
    cSubP->attributes.erase(cSubP->attributes.begin());
  }
  cSubP->attributes.clear();

  cSubP->notifyConditionVector.release();
  cSubP->notifyConditionVector.vec.clear();

  cSubP->next = NULL;
}



/* ****************************************************************************
*
* mongoSubCacheDestroy - 
*/
void mongoSubCacheDestroy(void)
{
  LM_T(LmtMongoSubCache, ("destroying subscription cache"));

  CachedSubscription* cSubP  = mongoSubCache.head;

  if (mongoSubCache.head == NULL)
  {
    return;
  }

  while (cSubP->next)
  {
    CachedSubscription* prev = cSubP;

    cSubP = cSubP->next;
    cachedSubscriptionDestroy(prev);
    delete prev;
  }

  cachedSubscriptionDestroy(cSubP);
  delete cSubP;

  mongoSubCache.head  = NULL;
  mongoSubCache.tail  = NULL;
  mongoSubCache.items = 0;
}



/* ****************************************************************************
*
* tenantMatch - 
*/
static bool tenantMatch(const char* tenant1, const char* tenant2)
{
  if ((tenant1 == NULL) && (tenant2 == NULL))
  {
    return true;
  }

  if ((tenant1 == NULL) || (tenant2 == NULL))
  {
    return false;
  }

  if (strcmp(tenant1, tenant2) == 0)
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* mongoSubCacheItemLookup - 
*
* FIXME P7: lookups would be A LOT faster if the subCache used a hash-table instead of
*           just a single linked link for the subscriptions.
*           The hash could be adding each char in 'tenant' and 'subscriptionId' to a 8-bit
*           integer and then we'd have a vector of 256 linked lists.
*/
CachedSubscription* mongoSubCacheItemLookup(const char* tenant, const char* subscriptionId)
{
  CachedSubscription* cSubP = mongoSubCache.head;

  while (cSubP != NULL)
  {
    if ((tenantMatch(tenant, cSubP->tenant)) && (strcmp(subscriptionId, cSubP->subscriptionId) == 0))
    {
      return cSubP;
    }

    cSubP = cSubP->next;
  }

  return NULL;
}



/* ****************************************************************************
*
* mongoSubCacheItemInsert - 
*
* RETURN VALUES
*   0:  all OK
*  -1:  Database Error - id-field not found
*  -2:  Out of memory (either returns -2 or exit the entire broker)
*  -3:  The vector of notify-conditions is empty
*
*/
int mongoSubCacheItemInsert(const char* tenant, const BSONObj& sub)
{
  //
  // 01. Check validity of subP parameter 
  //
  BSONElement  idField = sub.getField("_id");

  if (idField.eoo() == true)
  {
    LM_E(("Database Error (error retrieving _id field in doc: '%s')", sub.toString().c_str()));
    return -1;
  }


  //
  // 03. Create CachedSubscription
  //
  CachedSubscription* cSubP = new CachedSubscription();

  if (cSubP == NULL)
  {
    // FIXME P7: See github issue #1362
    LM_X(1, ("Runtime Error (cannot allocate memory for a cached subscription: %s)", strerror(errno)));
    return -2;
  }


  //
  // 04. Extract data from subP
  //
  std::string               formatString  = sub.hasField(CSUB_FORMAT)? sub.getField(CSUB_FORMAT).String() : "XML";
  std::vector<BSONElement>  eVec          = sub.getField(CSUB_ENTITIES).Array();
  std::vector<BSONElement>  attrVec       = sub.getField(CSUB_ATTRS).Array();
  std::vector<BSONElement>  condVec       = sub.getField(CSUB_CONDITIONS).Array();


  cSubP->tenant = (tenant[0] == 0)? NULL : strdup(tenant);

  //
  // FIXME: Check all strings about empty strings
  //        What happens if you call strdup on an empty string?
  //
  cSubP->subscriptionId        = strdup(idField.OID().toString().c_str());
  cSubP->servicePath           = strdup(sub.hasField(CSUB_SERVICE_PATH)? sub.getField(CSUB_SERVICE_PATH).String().c_str() : "/");
  cSubP->reference             = strdup(sub.hasField(CSUB_REFERENCE)?    sub.getField(CSUB_REFERENCE).String().c_str() : "NO REF");  // Mandatory
  cSubP->notifyFormat          = stringToFormat(formatString);
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)? sub.getField(CSUB_THROTTLING).Long() : -1;
  cSubP->expirationTime        = sub.hasField(CSUB_EXPIRATION)? sub.getField(CSUB_EXPIRATION).Long() : 0;
  cSubP->lastNotificationTime  = sub.hasField(CSUB_LASTNOTIFICATION)? sub.getField(CSUB_LASTNOTIFICATION).Int() : 0;
  cSubP->pendingNotifications  = 0;
  cSubP->next                  = NULL;

  LM_T(LmtMongoSubCache, ("set lastNotificationTime to %lu for '%s' (from DB)", cSubP->lastNotificationTime, cSubP->subscriptionId));

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

    std::string id = entity.getStringField(ENT_ENTITY_ID);
    
    if (!entity.hasField(CSUB_ENTITY_ISPATTERN))
    {
      continue;
    }

    std::string isPattern = entity.getStringField(CSUB_ENTITY_ISPATTERN);
    if (isPattern != "true")
    {
      continue;
    }

    std::string type;
    if (entity.hasField(CSUB_ENTITY_TYPE))
    {
      type = entity.getStringField(CSUB_ENTITY_TYPE);
    }

    EntityInfo* eiP = new EntityInfo(id, type);
    cSubP->entityIdInfos.push_back(eiP);  // definitely lost ... why?
  }


  //
  // 06. Push attribute names to Attribute Vector (cSubP->attributes)
  //
  for (unsigned int ix = 0; ix < attrVec.size(); ++ix)
  {
    std::string s = attrVec[ix].String();
    cSubP->attributes.push_back(s);  // definitely lost, same ... why?
  }



  //
  // 07. Fill in cSubP->notifyConditionVector from condVec
  //
  for (unsigned int ix = 0; ix < condVec.size(); ++ix)
  {
    BSONObj                   condition = condVec[ix].embeddedObject();
    std::string               condType;
    std::vector<BSONElement>  valueVec;

    condType = condition.getStringField(CSUB_CONDITIONS_TYPE);
    if (condType != "ONCHANGE")
    {
      continue;
    }

    NotifyCondition* ncP = new NotifyCondition();
    ncP->type = condType;

    valueVec = condition.getField(CSUB_CONDITIONS_VALUE).Array();
    for (unsigned int vIx = 0; vIx < valueVec.size(); ++vIx)
    {
      std::string condValue;

      condValue = valueVec[vIx].String();
      ncP->condValueList.push_back(condValue);
    }

    cSubP->notifyConditionVector.push_back(ncP);  // definitely lost (inside NotifyConditionVector.cpp:113)
  }

  if (cSubP->notifyConditionVector.size() == 0)  // Cleanup
  {
    LM_E(("ERROR (empty notifyConditionVector) - cleaning up"));
    cachedSubscriptionDestroy(cSubP);
    return -3;
  }

  mongoSubCacheItemInsert(cSubP);

  return 0;
}



/* ****************************************************************************
*
* mongoSubCacheItemInsert - create a new sub, fill it in, and add it to cache
*/
void mongoSubCacheItemInsert
(
  const char*               tenant,
  const char*               servicePath,
  SubscribeContextRequest*  scrP,
  const char*               subscriptionId,
  int64_t                   expirationTime,
  int64_t                   throttling,
  Format                    notifyFormat
)
{
  CachedSubscription* cSubP = new CachedSubscription();

  //
  // 1. First the non-complex values
  //
  cSubP->tenant                = (tenant[0] == 0)? NULL : strdup(tenant);
  cSubP->servicePath           = strdup(servicePath);
  cSubP->subscriptionId        = strdup(subscriptionId);
  cSubP->reference             = strdup(scrP->reference.get().c_str());
  cSubP->expirationTime        = expirationTime;
  cSubP->throttling            = throttling;
  cSubP->lastNotificationTime  = 0;
  cSubP->pendingNotifications  = 0;
  cSubP->notifyFormat          = notifyFormat;
  cSubP->next                  = NULL;

  LM_T(LmtMongoSubCache, ("inserting a new sub in cache (%s). lastNotifictionTime: %lu", cSubP->subscriptionId, cSubP->lastNotificationTime));


  //
  // 2. Then the values that have functions/methods for filling/parsing
  //
  cSubP->throttling = scrP->throttling.parse();
  cSubP->notifyConditionVector.fill(scrP->notifyConditionVector);


  //
  // 3. Convert all patterned EntityId to EntityInfo
  //
  for (unsigned int ix = 0; ix < scrP->entityIdVector.vec.size(); ++ix)
  {
    EntityId* eIdP;

    eIdP = scrP->entityIdVector.vec[ix];

    if (eIdP->isPattern == "false")
    {
      continue;
    }

    EntityInfo* eP = new EntityInfo(eIdP->id, eIdP->type);
    
    cSubP->entityIdInfos.push_back(eP);
  }


  //
  // 4. Insert all attributes 
  //
  for (unsigned int ix = 0; ix < scrP->attributeList.attributeV.size(); ++ix)
  {
    cSubP->attributes.push_back(scrP->attributeList.attributeV[ix]);
  }


  //
  // 5. Now, insert the subscription in the cache
  //
  LM_M(("Inserting NEW sub '%s', lastNotificationTime: %lu", cSubP->subscriptionId, cSubP->lastNotificationTime));
  mongoSubCacheItemInsert(cSubP);

  mongoSubCache.noOfInserts  += 1;
}



/* ****************************************************************************
*
* mongoSubCacheStatisticsGet - 
*/
void mongoSubCacheStatisticsGet(int* refreshes, int* inserts, int* removes, int* items)
{
  *refreshes = mongoSubCache.noOfRefreshes;
  *inserts   = mongoSubCache.noOfInserts;
  *removes   = mongoSubCache.noOfRemoves;
  *items     = mongoSubCache.items;
}



/* ****************************************************************************
*
* mongoSubCacheStatisticsReset - 
*/
void mongoSubCacheStatisticsReset(void)
{
  mongoSubCache.noOfRefreshes  = 0;
  mongoSubCache.noOfInserts    = 0;
  mongoSubCache.noOfRemoves    = 0;
}



/* ****************************************************************************
*
* mongoSubCachePresent - 
*/
void mongoSubCachePresent(const char* title)
{
  CachedSubscription* cSubP = mongoSubCache.head;

  LM_T(LmtMongoSubCache, ("----------- %s ------------", title));

  while (cSubP != NULL)
  {
    LM_T(LmtMongoSubCache, ("o %s (tenant: %s)", cSubP->subscriptionId, cSubP->tenant));
    cSubP = cSubP->next;
  }

  LM_T(LmtMongoSubCache, ("--------------------------------"));
}

/* ****************************************************************************
*
* mongoSubCacheItemRemove - 
*/
int mongoSubCacheItemRemove(CachedSubscription* cSubP)
{
  CachedSubscription* current = mongoSubCache.head;
  CachedSubscription* prev    = NULL;

  LM_T(LmtMongoSubCache, ("in mongoSubCacheItemRemove, trying to remove '%s'", cSubP->subscriptionId));
  while (current != NULL)
  {
    if (current == cSubP)
    {
      if (cSubP == mongoSubCache.head)  mongoSubCache.head = cSubP->next;
      if (cSubP == mongoSubCache.tail)  mongoSubCache.tail = prev;
      if (prev != NULL)                 prev->next         = cSubP->next;

      LM_T(LmtMongoSubCache, ("in mongoSubCacheItemRemove, REMOVING '%s'", cSubP->subscriptionId));
      mongoSubCache.noOfRemoves += 1;

      cachedSubscriptionDestroy(cSubP);
      return 0;
    }

    prev    = current;
    current = current->next;
  }

  LM_E(("Runtime Error (item to remove from sub-cache not found)"));
  return -1;
}



/* ****************************************************************************
*
* mongoSubCacheRefresh -
*
* 1. Empty cache
* 2. Lookup all subscriptions in the database 
* 3. Insert them again in the cache (with fresh data from database)
*/
static void mongoSubCacheRefresh(std::string database)
{
  LM_T(LmtMongoSubCache, ("in mongoSubCacheRefresh"));

  BSONObj                   query      = BSON("conditions.type" << "ONCHANGE" << CSUB_ENTITIES "." CSUB_ENTITY_ISPATTERN << "true");
  DBClientBase*             connection = getMongoConnection();
  auto_ptr<DBClientCursor>  cursor;

  std::string tenant = tenantFromDb(database);
  try
  {
    cursor = connection->query(getSubscribeContextCollectionName(tenant).c_str(), query);

    /*
     * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
     * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
     * exception ourselves
     */
    if (cursor.get() == NULL)
    {
      throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    releaseMongoConnection(connection);

    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);
    LM_E(("Database Error (DBException: %s)", e.what()));
    return;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    LM_E(("Database Error (generic exception)"));
    return;
  }

  // Call the treat function for each subscription
  int subNo = 0;
  while (cursor->more())
  {
    BSONObj sub = cursor->next();

    mongoSubCacheItemInsert(tenant.c_str(), sub);

    ++subNo;
  }
}



/* ****************************************************************************
*
* mongoSubCacheRefresh - 
*/
void mongoSubCacheRefresh(void)
{
  std::vector<std::string> databases;
  bool                     reqSemTaken;

  reqSemTake(__FUNCTION__, "subscription cache", SemReadOp, &reqSemTaken);

  // Empty the cache
  mongoSubCacheDestroy();

  // Get list of database
  getOrionDatabases(databases);

  // Add the 'default tenant'
  databases.push_back(dbPrefixGet());


  // Now refresh the subCache for each and every tenant
  for (unsigned int ix = 0; ix < databases.size(); ++ix)
  {
    mongoSubCacheRefresh(databases[ix]);
  }

  ++mongoSubCache.noOfRefreshes;
  reqSemGive(__FUNCTION__, "subscription cache", reqSemTaken);
}



/* ****************************************************************************
*
* mongoSubCacheRefresherThread - 
*/
static void* mongoSubCacheRefresherThread(void* vP)
{
  extern int subCacheInterval;

  while (1)
  {
    mongoSubCacheRefresh();
    sleep(subCacheInterval);
  }

  return NULL;
}



/* ****************************************************************************
*
* mongoSubCacheStart - 
*/
void mongoSubCacheStart(void)
{
  pthread_t tid;
  int ret = pthread_create(&tid, NULL, mongoSubCacheRefresherThread, NULL);

  if (ret != 0)
  {
    LM_E(("Runtime Error (error creating thread: %d)", ret));
    return;
  }
  pthread_detach(tid);
}
