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

/*
*
* The mongo subscription cache maintains in memory all subscriptions of type ONCHANGE that has
* a patterned Entity::id. The reason for this cache is to avoid a very slow mongo operation ... (Fermin)
*
* The cache is implementged in mongoBackend/mongoSubCache.cpp/h and used in:
*   - MongoCommonUpdate.cpp               (function addTriggeredSubscriptions_withCache)
*   - mongoSubscribeContext.cpp           (function mongoSubscribeContext)
*   - mongoUnsubscribeContext.cpp         (function mongoUnsubscribeContext)
*   - mongoUpdateContextSubscription.cpp  (function mongoUpdateContextSubscription)
*   - mongoSubCache.cpp                   (function mongoSubCacheRefresh)
*
* To manipulate the subscription cache, a semaphore is necessary, as various threads can be
* reading/inserting/removing subs at the same time.
* This semaphore is NOT optional, like the mongo request semaphore.
*
* Two functions have been added to common/sem.cpp|h for this: cacheSemTake/Give.
*/
#include <string>
#include <regex.h>

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "common/string.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/mongoSubCache.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"

#include "ngsi/NotifyConditionVector.h"
#include "ngsi10/SubscribeContextRequest.h"

using namespace mongo;
using std::map;



/* ****************************************************************************
*
* EntityInfo::EntityInfo - 
*/
EntityInfo::EntityInfo(const std::string& _entityId, const std::string& _entityType, const std::string& _isPattern)
{
  entityId     = _entityId;
  entityType   = _entityType;
  isPattern    = (_isPattern == "true") || (_isPattern == "TRUE") || (_isPattern == "True");

  if (isPattern)
  {
    regcomp(&entityIdPattern, _entityId.c_str(), 0);
    entityIdPatternToBeFreed = true;
  }
  else
  {
    entityIdPatternToBeFreed = false;
  }
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

  if (isPattern)
  {
    // REGEX-comparison this->entityIdPattern VS id
    return regexec(&entityIdPattern, id.c_str(), 0, NULL, 0) == 0;
  }

  return (id == entityId);
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
<<<<<<< HEAD
  LM_T(LmtPresent, ("%sid:    regex describing EntityId::id (the original string is not saved)", prefix.c_str()));
  LM_T(LmtPresent, ("%stype:  %s", prefix.c_str(), entityType.c_str()));
=======
  LM_F(("%sid:        %s", prefix.c_str(), entityId.c_str()));
  LM_F(("%sisPattern: %s", prefix.c_str(), FT(isPattern)));
  LM_F(("%stype:      %s", prefix.c_str(), entityType.c_str()));
>>>>>>> develop
}



/* ****************************************************************************
*
* MongoSubCache - 
*/
typedef struct MongoSubCache
{
  CachedSubscription* head;
  CachedSubscription* tail;

  // Statistics counters
  int                 noOfRefreshes;
  int                 noOfInserts;
  int                 noOfRemoves;
  int                 noOfUpdates;
} MongoSubCache;



/* ****************************************************************************
*
* mongoSubCache - 
*/
MongoSubCache  mongoSubCache       = { NULL, NULL, 0 };
bool           mongoSubCacheActive = false;



/* ****************************************************************************
*
* mongoSubCacheInit - 
*/
void mongoSubCacheInit(void)
{
  LM_T(LmtMongoSubCache, ("Initializing subscription cache"));

  mongoSubCache.head   = NULL;
  mongoSubCache.tail   = NULL;

  mongoSubCacheStatisticsReset("mongoSubCacheInit");

  mongoSubCacheActive = true;
}



/* ****************************************************************************
*
* mongoSubCacheItems - 
*/
int mongoSubCacheItems(void)
{
  CachedSubscription* cSubP = mongoSubCache.head;
  int                 items = 0;

  while (cSubP != NULL)
  {
    ++items;
    cSubP = cSubP->next;
  }

  return items;
}



/* ****************************************************************************
*
* attributeMatch - 
*/
static bool attributeMatch(CachedSubscription* cSubP, const std::vector<std::string>& attrV)
{
  for (unsigned int ncvIx = 0; ncvIx < cSubP->notifyConditionVector.size(); ++ncvIx)
  {
    NotifyCondition* ncP = cSubP->notifyConditionVector[ncvIx];

    for (unsigned int cvIx = 0; cvIx < ncP->condValueList.size(); ++cvIx)
    {
      for (unsigned int aIx = 0; aIx < attrV.size(); ++aIx)
      {
        if (ncP->condValueList[cvIx] == attrV[aIx])
        {
          return true;
        }
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
  CachedSubscription*              cSubP,
  const char*                      tenant,
  const char*                      servicePath,
  const char*                      entityId,
  const char*                      entityType,
  const std::vector<std::string>&  attrV
)
{
  if ((cSubP->tenant == NULL) || (tenant == NULL) || (cSubP->tenant[0] == 0) || (tenant[0] == 0))
  {
    if ((cSubP->tenant != NULL) && (cSubP->tenant[0] != 0))
    {
      LM_T(LmtMongoSubCacheMatch, ("No match due to tenant I"));
      return false;
    }

    if ((tenant != NULL) && (tenant[0] != 0))
    {
      LM_T(LmtMongoSubCacheMatch, ("No match due to tenant II"));
      return false;
    }
  }
  else if (strcmp(cSubP->tenant, tenant) != 0)
  {
    LM_T(LmtMongoSubCacheMatch, ("No match due to tenant III"));
    return false;
  }

  if (servicePathMatch(cSubP, (char*) servicePath) == false)
  {
    LM_T(LmtMongoSubCacheMatch, ("No match due to servicePath"));
    return false;
  }


  //
  // If ONCHANGE and one of the attribute names in the scope vector
  // of the subscription has the same name as the incoming attribute. there is a match.
  //
  if (!attributeMatch(cSubP, attrV))
  {
    LM_T(LmtMongoSubCacheMatch, ("No match due to attributes"));
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

  LM_T(LmtMongoSubCacheMatch, ("No match due to EntityInfo"));
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
    std::vector<std::string> attrV;

    attrV.push_back(attr);

    if (subMatch(cSubP, tenant, servicePath, entityId, entityType, attrV))
    {
      subVecP->push_back(cSubP);
      LM_T(LmtMongoSubCache, ("added subscription '%s': lastNotificationTime: %lu", cSubP->subscriptionId, cSubP->lastNotificationTime));
    }

    cSubP = cSubP->next;
  }
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
  const std::vector<std::string>&    attrV,
  std::vector<CachedSubscription*>*  subVecP
)
{
  CachedSubscription* cSubP = mongoSubCache.head;

  while (cSubP != NULL)
  {
    if (subMatch(cSubP, tenant, servicePath, entityId, entityType, attrV))
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
    LM_T(LmtMongoSubCache,  ("removing CachedSubscription at %p", prev));
    delete prev;
  }

  cachedSubscriptionDestroy(cSubP);
  LM_T(LmtMongoSubCache,  ("removing CachedSubscription at %p", cSubP));
  delete cSubP;

  mongoSubCache.head  = NULL;
  mongoSubCache.tail  = NULL;
}



/* ****************************************************************************
*
* tenantMatch - 
*/
static bool tenantMatch(const char* tenant1, const char* tenant2)
{
  //
  // Removing complications with NULL, giving NULL tenants the value of an empty string;
  //
  tenant1 = (tenant1 == NULL)? "" : tenant1;
  tenant2 = (tenant2 == NULL)? "" : tenant2;

  if (strlen(tenant1) != strlen(tenant2))
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
*           Another way of implementing this would be to use a std::vector or std::map.
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
* mongoSubCacheUpdateStatisticsIncrement - 
*/
void mongoSubCacheUpdateStatisticsIncrement(void)
{
  ++mongoSubCache.noOfUpdates;
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

  ++mongoSubCache.noOfInserts;

  // First insertion?
  if ((mongoSubCache.head == NULL) && (mongoSubCache.tail == NULL))
  {
    mongoSubCache.head   = cSubP;
    mongoSubCache.tail   = cSubP;

    return;
  }

  mongoSubCache.tail->next  = cSubP;
  mongoSubCache.tail        = cSubP;
}



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
  LM_T(LmtMongoSubCache,  ("allocated CachedSubscription at %p", cSubP));

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


  cSubP->tenant                = (tenant[0] == 0)? strdup("") : strdup(tenant);
  cSubP->subscriptionId        = strdup(idField.OID().toString().c_str());
  cSubP->servicePath           = strdup(sub.hasField(CSUB_SERVICE_PATH)? sub.getField(CSUB_SERVICE_PATH).String().c_str() : "/");
  cSubP->reference             = strdup(sub.hasField(CSUB_REFERENCE)?    sub.getField(CSUB_REFERENCE).String().c_str() : "NO REF");  // Mandatory
  cSubP->notifyFormat          = stringToFormat(formatString);
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLong(sub, CSUB_THROTTLING)       : -1;
  cSubP->expirationTime        = sub.hasField(CSUB_EXPIRATION)?       getIntOrLongFieldAsLong(sub, CSUB_EXPIRATION)       : 0;
  cSubP->lastNotificationTime  = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLong(sub, CSUB_LASTNOTIFICATION) : -1;
  cSubP->count                 = sub.hasField(CSUB_COUNT)?            getIntOrLongFieldAsLong(sub, CSUB_COUNT)            : 0;
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

    std::string id        = entity.getStringField(ENT_ENTITY_ID);
    std::string isPattern = entity.hasField(CSUB_ENTITY_ISPATTERN)? entity.getStringField(CSUB_ENTITY_ISPATTERN) : "false";
    std::string type      = entity.hasField(CSUB_ENTITY_TYPE)?      entity.getStringField(CSUB_ENTITY_TYPE)      : "";
    EntityInfo* eiP       = new EntityInfo(id, type, isPattern);

    cSubP->entityIdInfos.push_back(eiP);
  }

  if (cSubP->entityIdInfos.size() == 0)
  {
    LM_E(("ERROR (no patterned entityId) - cleaning up"));
    cachedSubscriptionDestroy(cSubP);
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

    cSubP->notifyConditionVector.push_back(ncP);
  }

  if (cSubP->notifyConditionVector.size() == 0)  // Cleanup
  {
    LM_E(("ERROR (empty notifyConditionVector) - cleaning up"));
    cachedSubscriptionDestroy(cSubP);
    delete cSubP;
    return -4;
  }

  mongoSubCacheItemInsert(cSubP);

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
*/
int mongoSubCacheItemInsert
(
  const char*     tenant,
  const BSONObj&  sub,
  const char*     subscriptionId,
  const char*     servicePath,
  int             lastNotificationTime,
  long long       expirationTime
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

  LM_T(LmtMongoSubCache,  ("allocated CachedSubscription at %p", cSubP));

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
  std::vector<BSONElement>  eVec = sub.getField(CSUB_ENTITIES).Array();

  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField(CSUB_ENTITY_ID))
    {
      LM_W(("Runtime Error (got a subscription without id)"));
      continue;
    }

    std::string id        = entity.getStringField(ENT_ENTITY_ID);
    std::string isPattern = entity.hasField(CSUB_ENTITY_ISPATTERN)? entity.getStringField(CSUB_ENTITY_ISPATTERN) : "false";
    std::string type      = entity.hasField(CSUB_ENTITY_TYPE)?      entity.getStringField(CSUB_ENTITY_TYPE)      : "";
    EntityInfo* eiP       = new EntityInfo(id, type, isPattern);

    cSubP->entityIdInfos.push_back(eiP);
  }

  if (cSubP->entityIdInfos.size() == 0)
  {
    cachedSubscriptionDestroy(cSubP);
    delete cSubP;
    return -4;
  }


  //
  // 03. Extract data from mongo sub
  //
  std::string               formatString  = sub.hasField(CSUB_FORMAT)? sub.getField(CSUB_FORMAT).String() : "XML";
  std::vector<BSONElement>  attrVec       = sub.getField(CSUB_ATTRS).Array();
  std::vector<BSONElement>  condVec       = sub.getField(CSUB_CONDITIONS).Array();

  if ((lastNotificationTime == -1) && (sub.hasField(CSUB_LASTNOTIFICATION)))
  {
    //
    // If no lastNotificationTime is given to this function AND
    // if the database objuect contains lastNotificationTime,
    // then use the value from the database
    //
    lastNotificationTime = getIntOrLongFieldAsLong(sub, CSUB_LASTNOTIFICATION);
  }

  cSubP->tenant                = (tenant[0] == 0)? NULL : strdup(tenant);
  cSubP->subscriptionId        = strdup(subscriptionId);
  cSubP->servicePath           = strdup(servicePath);
  cSubP->notifyFormat          = stringToFormat(formatString);
  cSubP->reference             = strdup(sub.hasField(CSUB_REFERENCE)? sub.getField(CSUB_REFERENCE).String().c_str() : "NO REF");  // Mandatory
  cSubP->throttling            = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLong(sub, CSUB_THROTTLING) : -1;
  cSubP->expirationTime        = expirationTime;
  cSubP->lastNotificationTime  = lastNotificationTime;
  cSubP->next                  = NULL;

  LM_T(LmtMongoSubCache, ("set lastNotificationTime to %lu for '%s' (from DB)", cSubP->lastNotificationTime, cSubP->subscriptionId));


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

    cSubP->notifyConditionVector.push_back(ncP);
  }

  if (cSubP->notifyConditionVector.size() == 0)
  {
    cachedSubscriptionDestroy(cSubP);
    delete cSubP;
    return -5;
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
  Format                    notifyFormat,
  bool                      notificationDone,
  int64_t                   lastNotificationTime
)
{
  //
  // Add the subscription to the subscription cache.
  // But only if any of the entities in entityIdVector is pattern based -
  // AND it is a subscription of type ONCHANGE
  //

  //
  // 00. Check that the subscription is pattern based an ONCHANGE
  //
  bool onchange = false;

  for (unsigned int ix = 0; ix < scrP->notifyConditionVector.size(); ++ix)
  {
    if (strcasecmp(scrP->notifyConditionVector[ix]->type.c_str(), ON_CHANGE_CONDITION) == 0)
    {
      onchange = true;
      break;
    }
  }

  if (!onchange)
  {
    return;
  }

    
  //
  // Now allocate the subscription
  //
  CachedSubscription* cSubP = new CachedSubscription();
  LM_T(LmtMongoSubCache,  ("allocated CachedSubscription at %p", cSubP));


  //
  // 1. First the non-complex values
  //
  cSubP->tenant                = (tenant[0] == 0)? NULL : strdup(tenant);
  cSubP->servicePath           = strdup(servicePath);
  cSubP->subscriptionId        = strdup(subscriptionId);
  cSubP->reference             = strdup(scrP->reference.get().c_str());
  cSubP->expirationTime        = expirationTime;
  cSubP->throttling            = throttling;
  cSubP->lastNotificationTime  = lastNotificationTime;
  cSubP->notifyFormat          = notifyFormat;
  cSubP->next                  = NULL;
  cSubP->count                 = (notificationDone == true)? 1 : 0;

  LM_T(LmtMongoSubCache, ("inserting a new sub in cache (%s). lastNotifictionTime: %lu", cSubP->subscriptionId, cSubP->lastNotificationTime));


  //
  // 2. Then the values that have functions/methods for filling/parsing
  //
  cSubP->throttling = scrP->throttling.parse();
  cSubP->notifyConditionVector.fill(scrP->notifyConditionVector);


  //
  // 3. Convert all EntityIds to EntityInfo
  //
  for (unsigned int ix = 0; ix < scrP->entityIdVector.vec.size(); ++ix)
  {
    EntityId* eIdP;

    eIdP = scrP->entityIdVector.vec[ix];

    EntityInfo* eP = new EntityInfo(eIdP->id, eIdP->type, eIdP->isPattern);
    
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
  LM_T(LmtMongoSubCache, ("Inserting NEW sub '%s', lastNotificationTime: %lu", cSubP->subscriptionId, cSubP->lastNotificationTime));
  mongoSubCacheItemInsert(cSubP);
}



/* ****************************************************************************
*
* mongoSubCacheStatisticsGet - 
*/
void mongoSubCacheStatisticsGet(int* refreshes, int* inserts, int* removes, int* updates, int* items, char* list, int listSize)
{
  *refreshes = mongoSubCache.noOfRefreshes;
  *inserts   = mongoSubCache.noOfInserts;
  *removes   = mongoSubCache.noOfRemoves;
  *updates   = mongoSubCache.noOfUpdates;
  *items     = mongoSubCacheItems();

  CachedSubscription* cSubP = mongoSubCache.head;

  //
  // NOTE
  //   If the listBuffer is not big enough to hold the entire list of cached subscriptions,
  //   mongoSubCacheStatisticsGet returns the empty string and the list is excluded from
  //   the response of "GET /statistics".
  //   Minimum size of the list is set to 128 bytes and the needed size is detected later.
  //   Again, if the list-buffer is not big enough to hold the entire text, no list is shown.
  //
  *list = 0;
  if (listSize > 128)
  {
    while (cSubP != NULL)
    {
      char          msg[256];
      unsigned int  bytesLeft = listSize - strlen(list);

#if 0
      //
      // FIXME P5: To be removed once sub-update is OK in QA
      //
      int now = getCurrentTime();
      snprintf(msg, sizeof(msg), "%s|N:%lu|E:%lu|T:%lu|DUR:%lu",
               cSubP->subscriptionId,
               cSubP->lastNotificationTime,
               cSubP->expirationTime,
               cSubP->throttling,
               cSubP->expirationTime - now);
#else
        snprintf(msg, sizeof(msg), "%s", cSubP->subscriptionId);
#endif
      //
      // If "msg" and ", " has no room in the "list", then no list is shown.
      // (the '+ 2' if for the string ", ")
      //
      if (strlen(msg) + 2 > bytesLeft)
      {
        *list = 0;
        break;
      }
      
      if (list[0] != 0)
        strcat(list, ", ");

      strcat(list, msg);

      cSubP = cSubP->next;
    }
  }

  LM_T(LmtMongoSubCache, ("Statistics: refreshes: %d (%d)", *refreshes, mongoSubCache.noOfRefreshes));
}



/* ****************************************************************************
*
* mongoSubCacheStatisticsReset - 
*/
void mongoSubCacheStatisticsReset(const char* by)
{
  mongoSubCache.noOfRefreshes  = 0;
  mongoSubCache.noOfInserts    = 0;
  mongoSubCache.noOfRemoves    = 0;
  mongoSubCache.noOfUpdates    = 0;

  LM_T(LmtMongoSubCache, ("Statistics reset by %s: refreshes is now %d", by, mongoSubCache.noOfRefreshes));
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
    LM_T(LmtMongoSubCache, ("o %s (tenant: %s, LNT: %lu, THR: %d)", cSubP->subscriptionId, cSubP->tenant, cSubP->lastNotificationTime, cSubP->throttling));
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
      if (cSubP == mongoSubCache.head)  { mongoSubCache.head = cSubP->next; }
      if (cSubP == mongoSubCache.tail)  { mongoSubCache.tail = prev;        }
      if (prev != NULL)                 { prev->next         = cSubP->next; }

      LM_T(LmtMongoSubCache, ("in mongoSubCacheItemRemove, REMOVING '%s'", cSubP->subscriptionId));
      ++mongoSubCache.noOfRemoves;

      cachedSubscriptionDestroy(cSubP);
      delete cSubP;

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
*
* NOTE
*   The query for the database ONLY extracts the interesting subscriptions:
*   - "conditions.type" << "ONCHANGE"
*
*   I.e. the subscriptions is for ONCHANGE.
*/
static void mongoSubCacheRefresh(const std::string& database)
{
  LM_T(LmtMongoSubCache, ("Refreshing subscription cache for DB '%s'", database.c_str()));

  BSONObj                   query       = BSON("conditions.type" << "ONCHANGE");
  std::string               db          = database;
  std::string               tenant      = tenantFromDb(db);
  std::string               collection  = getSubscribeContextCollectionName(tenant);
  auto_ptr<DBClientCursor>  cursor;
  std::string               errorString;

  if (collectionQuery(collection, query, &cursor, &errorString) != true)
  {
    LM_E(("Database Error (%s)", errorString.c_str()));
    return;
  }

  // Call the treat function for each subscription
  int subNo = 0;
  while (moreSafe(cursor))
  {
    BSONObj      sub;
    std::string  err;

    if (!nextSafeOrError(cursor, &sub, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s", err.c_str()));
      continue;
    }

    int r = mongoSubCacheItemInsert(tenant.c_str(), sub);

    if (r == 0)
    {
      ++subNo;
    }
  }

  LM_T(LmtMongoSubCache, ("Added %d subscriptions for database '%s'", subNo, database.c_str()));
}



/* ****************************************************************************
*
* mongoSubCacheRefresh - 
*
* WARNING
*  The cache semaphore must be taken before this function is called:
*    cacheSemTake(__FUNCTION__, "Reason");
*  And released after mongoSubCacheRefresh finishes, of course.
*/
void mongoSubCacheRefresh(void)
{
  std::vector<std::string> databases;

  LM_T(LmtMongoSubCache, ("Refreshing subscription cache"));

  // Empty the cache
  mongoSubCacheDestroy();

  // Get list of database
  getOrionDatabases(databases);

  // Add the 'default tenant'
  databases.push_back(getDbPrefix());


  // Now refresh the subCache for each and every tenant
  for (unsigned int ix = 0; ix < databases.size(); ++ix)
  {
    LM_T(LmtMongoSubCache, ("DB %d: %s", ix, databases[ix].c_str()));
    mongoSubCacheRefresh(databases[ix]);
  }

  ++mongoSubCache.noOfRefreshes;
  LM_T(LmtMongoSubCache, ("Refreshed subscription cache [%d]", mongoSubCache.noOfRefreshes));

  //
  // FIXME P5: Remove this debugging code once the refresh works again
  //
#if 0

  //
  // DEBUG STARTS HERE
  //
  CachedSubscription* cSubP = mongoSubCache.head;
  int                 items = 0;

  while (cSubP != NULL)
  {
    LM_T(LmtMongoSubCache, ("%s: TEN:%s, SPath:%s, THR:%lu, EXP:%lu, LNT:%lu REF:%s, Es:%d, As:%d, NCs:%d",
                            cSubP->subscriptionId,
                            cSubP->tenant,
                            cSubP->servicePath,
                            cSubP->throttling,
                            cSubP->expirationTime,
                            cSubP->lastNotificationTime,
                            cSubP->reference,
                            cSubP->entityIdInfos.size(),
                            cSubP->attributes.size(),
                            cSubP->notifyConditionVector.size()));

    cSubP = cSubP->next;
    ++items;
  }

  LM_T(LmtMongoSubCache, ("%d subs in sub.cache", items));

  //
  // DEBUG ENDS HERE
  //
#endif
}



/* ****************************************************************************
*
* CachedSubSaved - 
*/
typedef struct CachedSubSaved
{
  long long  lastNotificationTime;
  long long  count;
} CachedSubSaved;



/* ****************************************************************************
*
* mongoSubCacheSync - 
*
* 1. Save subscriptionId, lastNotificationTime, and count for all items in cache (savedSubV)
* 2. Refresh cache (count set to 0)
* 3. Compare lastNotificationTime in savedSubV with the new cache-contents and:
*    3.1 Update cache-items where 'saved lastNotificationTime' > 'cached lastNotificationTime'
*    3.2 Remember this more correct lastNotificationTime (must be flushed to mongo) -
*        by clearing out (set to 0) those lastNotificationTimes that are newer in cache
* 4. Update 'count' for each item in savedSubV where count != 0
* 5. Update 'lastNotificationTime' foreach item in savedSubV where lastNotificationTime != 0
* 6. Free the vector created in step 1 - savedSubV
*/
void mongoSubCacheSync(void)
{
  std::map<std::string, CachedSubSaved*> savedSubV;

  cacheSemTake(__FUNCTION__, "Synchronizing subscription cache");


  //
  // 1. Save subscriptionId, lastNotificationTime, and count for all items in cache
  //
  CachedSubscription* cSubP = mongoSubCache.head;

  while (cSubP != NULL)
  {
    CachedSubSaved* cssP       = new CachedSubSaved();

    cssP->lastNotificationTime = cSubP->lastNotificationTime;
    cssP->count                = cSubP->count;

    savedSubV[cSubP->subscriptionId]= cssP;
    cSubP = cSubP->next;
  }

  LM_T(LmtMongoCacheSync, ("Pushed back %d items to savedSubV", savedSubV.size()));


  //
  // 2. Refresh cache (count set to 0)
  //
  mongoSubCacheRefresh();


  //
  // 3. Compare lastNotificationTime in savedSubV with the new cache-contents
  //
  cSubP = mongoSubCache.head;
  while (cSubP != NULL)
  {
    CachedSubSaved* cssP = savedSubV[cSubP->subscriptionId];
    if ((cssP != NULL) && (cssP->lastNotificationTime <= cSubP->lastNotificationTime))
    {
      // cssP->lastNotificationTime older than what's currently in DB => throw away
      cssP->lastNotificationTime = 0;
    }
    cSubP = cSubP->next;
  }


  //
  // 4. Update 'count' for each item in savedSubV where count != 0
  // 5. Update 'lastNotificationTime' foreach item in savedSubV where lastNotificationTime != 0
  //
  cSubP = mongoSubCache.head;
  while (cSubP != NULL)
  {
    CachedSubSaved* cssP = savedSubV[cSubP->subscriptionId];

    if (cssP == NULL)
    {
      cSubP = cSubP->next;
      continue;
    }

    std::string  tenant      = (cSubP->tenant == NULL)? "" : cSubP->tenant;
    std::string  collection  = getSubscribeContextCollectionName(tenant);
    BSONObj      condition;
    BSONObj      update;
    std::string  err;

    if (cssP->count != 0)
    {
      // Update count
      condition = BSON("_id" << OID(cSubP->subscriptionId));
      update    = BSON("$inc" << BSON(CSUB_COUNT << cssP->count));
    }
    else if (cssP->lastNotificationTime != 0)
    {
      // Update lastNotificationTime
      condition = BSON("_id" << OID(cSubP->subscriptionId) << CSUB_LASTNOTIFICATION << BSON("$lt" << cssP->lastNotificationTime));
      update    = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << cssP->lastNotificationTime));
    }
    else
    {
      cSubP = cSubP->next;
      continue;
    }

    if (collectionUpdate(collection, condition, update, false, &err) != true)
    {
      LM_E(("Internal Error (error updating 'count' and 'lastNotification' for a subscription)"));
    }

    cSubP = cSubP->next;
  }


  //
  // 6. Free the vector savedSubV
  //
  for (std::map<std::string, CachedSubSaved*>::iterator it = savedSubV.begin(); it != savedSubV.end(); ++it)
  {
    delete it->second;
  }
  savedSubV.clear();


  cacheSemGive(__FUNCTION__, "Synchronizing subscription cache");
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
    sleep(subCacheInterval);
    mongoSubCacheSync();
  }

  return NULL;
}



/* ****************************************************************************
*
* mongoSubCacheStart - 
*/
void mongoSubCacheStart(void)
{
  pthread_t  tid;
  int        ret;

  // Populate subscription cache from database
  mongoSubCacheRefresh();

  ret = pthread_create(&tid, NULL, mongoSubCacheRefresherThread, NULL);

  if (ret != 0)
  {
    LM_E(("Runtime Error (error creating thread: %d)", ret));
    return;
  }
  pthread_detach(tid);
}
