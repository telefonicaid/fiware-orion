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

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubCache.h"

using namespace mongo;
using std::auto_ptr;



/* ****************************************************************************
*
* EntityInfo - 
*
* The struct fields:
* -------------------------------------------------------------------------------
* o entityIdPattern      regex describing EntityId::id (OMA NGSI type)
* o entityType           string containing the type of the EntityId
*
*/
typedef struct EntityInfo
{
  regex_t       entityIdPattern;
  std::string   entityType;
  bool          entityIdPatternToBeFreed;

  EntityInfo() {}
  EntityInfo(const std::string& idPattern, const std::string& _entityType);
  ~EntityInfo() { release(); }

  bool          match(const std::string& idPattern, const std::string& type);
  void          release(void);
  void          present(const std::string& prefix);
} EntityInfo;



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
* CachedSubscription - 
*/
typedef struct CachedSubscription
{
  char*                       tenant;
  char*                       servicePath;
  char*                       subscriptionId;
  int64_t                     throttling;
  int64_t                     expirationTime;
  int64_t                     lastNotificationTime;
  Format                      format;
  char*                       reference;
  std::vector<EntityInfo*>    entityIdInfos;
  std::vector<std::string>    attributes;
  Restriction                 restriction;
  NotifyConditionVector       notifyConditionVector;
  struct CachedSubscription*  next;
} CachedSubscription;



/* ****************************************************************************
*
* MongoSubCache - 
*/
typedef struct MongoSubCache
{
  CachedSubscription* head;
  CachedSubscription* tail;
  int                 items;
} MongoSubCache;



/* ****************************************************************************
*
* mongoSubCache - 
*/
MongoSubCache mongoSubCache = { NULL, NULL, 0 };



/* ****************************************************************************
*
* mongoSubCacheInsert - 
*/
void mongoSubCacheInsert(CachedSubscription* cSubP)
{
  LM_M(("Inserting CachedSubscription at %p", cSubP));

  //
  // First of all, insertion is done at the end of the list, so, 
  // cSubP->next must ALWAYS be zero at insertion time
  //
  cSubP->next = NULL;


  // First insertion?
  if ((mongoSubCache.head == NULL) && (mongoSubCache.tail == NULL))
  {
    mongoSubCache.head  = cSubP;
    mongoSubCache.tail  = cSubP;
    mongoSubCache.items = 1;

    return;
  }

  mongoSubCache.tail->next  = cSubP;
  mongoSubCache.items      += 1;
}


/* ****************************************************************************
*
* cachedSubscriptionDestroy - 
*/
void cachedSubscriptionDestroy(CachedSubscription* cSubP)
{
  LM_M(("Destoying CachedSubscription at %p", cSubP));

  free(cSubP->tenant);
  free(cSubP->servicePath);
  free(cSubP->subscriptionId);

  free(cSubP->reference);

//  cSubP->entityIdInfos.release();
  cSubP->entityIdInfos.clear();

  cSubP->attributes.clear();
  cSubP->notifyConditionVector.release();

  cSubP->next = NULL;
}



/* ****************************************************************************
*
* mongoSubCacheDestroy - 
*/
void mongoSubCacheDestroy(void)
{
  CachedSubscription* cSubP = mongoSubCache.head;

  if (mongoSubCache.head == NULL)
  {
    return;
  }

  while (cSubP->next)
  {
    CachedSubscription* prev = cSubP;

    cSubP = cSubP->next;
    cachedSubscriptionDestroy(prev);
    free(prev);
  }

  cachedSubscriptionDestroy(cSubP);
  free(cSubP);

  mongoSubCache.head  = NULL;
  mongoSubCache.tail  = NULL;
  mongoSubCache.items = 0;
}



/* ****************************************************************************
*
* subCacheItemUpdate - 
*/
static void subCacheItemUpdate(const char* tenant, BSONObj* subP)
{
  //
  // 01. Check validity of subP parameter 
  //
  BSONElement  idField = subP->getField("_id");

  if (idField.eoo() == true)
  {
    LM_E(("Database Error (error retrieving _id field in doc: '%s')", subP->toString().c_str()));
    return;
  }


  //
  // 03. Create CachedSubscription
  //
  CachedSubscription* cSubP = (CachedSubscription*) calloc(1, sizeof(CachedSubscription));

  if (cSubP == NULL)
  {
    LM_E(("Runtime Error (cannot allocate memory for a cached subscription: %s)", strerror(errno)));
    return;
  }


  //
  // 04. Extract data from subP
  //
  std::string               formatString      = subP->hasField(CSUB_FORMAT)? subP->getField(CSUB_FORMAT).String() : "XML";
  std::vector<BSONElement>  eVec              = subP->getField(CSUB_ENTITIES).Array();
  std::vector<BSONElement>  attrVec           = subP->getField(CSUB_ATTRS).Array();
  std::vector<BSONElement>  condVec           = subP->getField(CSUB_CONDITIONS).Array();


  cSubP->tenant                = strdup(tenant);
  cSubP->subscriptionId        = strdup(idField.OID().toString().c_str());
  cSubP->servicePath           = strdup(subP->hasField(CSUB_SERVICE_PATH)? subP->getField(CSUB_SERVICE_PATH).String().c_str() : "/");
  cSubP->reference             = strdup(subP->hasField(CSUB_REFERENCE)?    subP->getField(CSUB_REFERENCE).String().c_str() : "NO REF");  // Mandatory
  cSubP->throttling            = subP->hasField(CSUB_THROTTLING)? subP->getField(CSUB_THROTTLING).Long() : -1;
  cSubP->expirationTime        = subP->hasField(CSUB_EXPIRATION)? subP->getField(CSUB_EXPIRATION).Long() : 0;
  cSubP->format                = stringToFormat(formatString);
  cSubP->lastNotificationTime  = subP->hasField(CSUB_LASTNOTIFICATION)? subP->getField(CSUB_LASTNOTIFICATION).Int() : 0;
  cSubP->next                  = NULL;

  // Debug counters
  int entities    = 0;
  int attributes  = 0;
  int conditions  = 0;


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
    cSubP->entityIdInfos.push_back(eiP);

    ++entities;
  }


  //
  // 06. Push attribute names to Attribute Vector (cSubP->attributes)
  //
  for (unsigned int ix = 0; ix < attrVec.size(); ++ix)
  {
    std::string attributeName = attrVec[ix].String();

    cSubP->attributes.push_back(attributeName);
    ++attributes;
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
      ++conditions;
    }

    cSubP->notifyConditionVector.push_back(ncP);
  }

  if (cSubP->notifyConditionVector.size() == 0)  // Cleanup
  {
    for (unsigned int ix = 0; ix < cSubP->entityIdInfos.size(); ++ix)
    {
      delete(cSubP->entityIdInfos[ix]);
    }
    cSubP->entityIdInfos.clear();

    return;
  }


#if 0
  //
  // 08. Debug
  //
  LM_M(("%s: TEN:%s, SPATH:%s, REF:%s, EXP:%lu, THR:%lu, FMT:%d, LNOT:%d, ENTS:%d, ATTR:%d,CONDS:%d",
        cSubP->subscriptionId,
        cSubP->tenant,
        cSubP->servicePath,
        cSubP->reference,
        cSubP->expirationTime,
        cSubP->throttling,
        cSubP->format,
        cSubP->lastNotificationTime,
        entities,
        attributes,
        conditions));
#endif
  mongoSubCacheInsert(cSubP);
}



/* ****************************************************************************
*
* mongoSubCacheRefresh -
*
* Lookup all subscriptions in the database and call a treat function for each
*/
static void mongoSubCacheRefresh(std::string database, MongoSubCacheTreat treatFunction)
{
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

  bool        reqSemTaken;
  reqSemTake(__FUNCTION__, "cache refresh", SemWriteOp, &reqSemTaken);

  // Call the treat function for each subscription
#if SUB_CACHE_ON
  subCache->semTake();
#endif
  int subNo = 0;
  while (cursor->more())
  {
    BSONObj sub = cursor->next();

    treatFunction(tenant.c_str(), &sub);

#if SUB_CACHE_ON
    treatFunction(tenant, &sub);
#endif

    ++subNo;
  }
#if SUB_CACHE_ON
  subCache->semGive();
#endif

  LM_M(("Got %d subscriptions for tenant '%s'", subNo, tenant.c_str()));
  
  reqSemGive(__FUNCTION__, "cache refresh", reqSemTaken);
}



/* ****************************************************************************
*
* mongoSubCacheRefresh - 
*/
void mongoSubCacheRefresh(void)
{
  std::vector<std::string> databases;
  static int               refreshNo = 0;

  ++refreshNo;
  LM_M(("Refreshing mongo subscription cache [%d]", refreshNo));
  LM_M(("First cached sub at %p", mongoSubCache.head));

  // Empty the cache
  mongoSubCacheDestroy();

  // Get list of database
  getOrionDatabases(databases);

  // Add the 'default tenant'
  databases.push_back(dbPrefixGet());

  // Now refresh the subCache for each and every tenant
  for (unsigned int ix = 0; ix < databases.size(); ++ix)
  {
    mongoSubCacheRefresh(databases[ix], subCacheItemUpdate);
  }

  LM_M(("Sub Cache refreshed: %d items in Sub Cache", mongoSubCache.items));
}
