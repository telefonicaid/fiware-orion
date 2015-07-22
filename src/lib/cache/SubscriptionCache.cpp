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
#include <semaphore.h>
#include <regex.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <time.h>

#include "logMsg/logMsg.h"

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "ngsi/Restriction.h"
#include "ngsi/Reference.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/EntityId.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "mongoBackend/MongoGlobal.h"
#include "cache/SubscriptionCache.h"



namespace orion
{


/* ****************************************************************************
*
* subCache - global singleton for the subscription cache
*/
SubscriptionCache* subCache = NULL;



/* ****************************************************************************
*
* subCacheMutexTime - 
*/
static struct timespec subCacheMutexTime = { 0, 0 };



/* ****************************************************************************
*
* subscriptionCacheInit - 
*/
void subscriptionCacheInit(std::string dbName)
{
  subCache = new SubscriptionCache(dbName);
  subCache->init();
}



/* ****************************************************************************
*
* subCacheMutexWaitingTimeGet - 
*/
void subCacheMutexWaitingTimeGet(char* buf, int bufLen)
{
  if (semTimeStatistics)
  {
    snprintf(buf, bufLen, "%lu.%09d", subCacheMutexTime.tv_sec, (int) subCacheMutexTime.tv_nsec);
  }
  else
  {
    snprintf(buf, bufLen, "Disabled");
  }
}



/* ****************************************************************************
*
* EntityInfo::EntityInfo - 
*/
EntityInfo::EntityInfo(const std::string& idPattern, const std::string& _entityType)
{
  regcomp(&entityIdPattern, idPattern.c_str(), 0);
  entityType = _entityType;
}



/* ****************************************************************************
*
* match - 
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
  if ((type != "") && (entityType != type))
  {
    return false;
  }

  // REGEX-comparison this->entityIdPattern VS id
  return regexec(&entityIdPattern, id.c_str(), 0, NULL, 0) == 0;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription()
{
  tenant                = "";
  servicePath           = "";
  subscriptionId        = "";
  throttling            = 0;
  expirationTime        = 0;
  lastNotificationTime  = -1;
  format                = XML;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription
(
  const std::string& _tenant,
  const std::string& _servicePath,
  const std::string& _subscriptionId,
  Format             _format
)
{
  tenant                = _tenant;
  servicePath           = _servicePath;
  subscriptionId        = _subscriptionId;
  throttling            = 0;
  expirationTime        = 0;
  lastNotificationTime  = -1;
  format                = _format;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription
(
  const std::string&        _tenant,
  const std::string&        _servicePath,
  SubscribeContextRequest*  scrP,
  std::string               _subscriptionId,
  int64_t                   _expiration,
  Format                    _format
)
{
  unsigned int ix;

  LM_M(("KZ: Creating Subscription from SubscribeContextRequest"));

  //
  // 0. Fill in 'constant initial values'
  //
  lastNotificationTime  = -1;
  format                = XML;



  //
  // 1. Get Tenant, ServicePath and Subscription Id
  //
  tenant         = _tenant;
  servicePath    = _servicePath;
  subscriptionId = _subscriptionId;
  format         = _format;

  //
  // 2. Convert all patterned EntityId to EntityInfo
  //
  for (ix = 0; ix < scrP->entityIdVector.vec.size(); ++ix)
  {
    EntityId* eIdP;

    eIdP = scrP->entityIdVector.vec[ix];

    if (eIdP->isPattern == "false")
    {
      continue;
    }

    LM_M(("KZ: Subscription: adding EntityInfo with pattern '%s'", eIdP->id.c_str()));
    EntityInfo* eP = new EntityInfo(eIdP->id, eIdP->type);
    
    entityIdInfoAdd(eP);
  }


  //
  // 3. Insert all attributes 
  //
  for (ix = 0; ix < scrP->attributeList.attributeV.size(); ++ix)
  {
    attributes.push_back(scrP->attributeList.attributeV[ix]);
  }


  //
  // 4. throttling
  //
  throttling = scrP->throttling.parse();


  //
  // 5. expirationTime
  //
  expirationTime = _expiration;


  //
  // 6. restriction
  //
  restriction.fill(&scrP->restriction);


  //
  // 7. notifyConditionVector
  //
  notifyConditionVector.fill(scrP->notifyConditionVector);
  LM_M(("Added %d notifyConditions", notifyConditionVector.size()));

  //
  // 8. reference
  //
  reference.set(scrP->reference.get());
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription
(
  const std::string&               _tenant,
  const std::string&               _servicePath,
  const std::string&               _subscriptionId,
  const std::vector<EntityInfo*>&  _entityIdInfos,
  const std::vector<std::string>&  _attributes,
  int64_t                          _throttling,
  int64_t                          _expirationTime,
  const Restriction&               _restriction,
  NotifyConditionVector&           _notifyConditionVector,
  const std::string&               _reference,
  Format                           _format
)
{
  unsigned int ix;

  tenant         = _tenant;
  servicePath    = _servicePath;
  subscriptionId = _subscriptionId;
  format         = _format;

  for (ix = 0; ix < _entityIdInfos.size(); ++ix)
  {
    entityIdInfos.push_back(_entityIdInfos[ix]);
  }

  for (ix = 0; ix < _attributes.size(); ++ix)
  {
    attributes.push_back(_attributes[ix]);
  }

  throttling     = _throttling;
  expirationTime = _expirationTime;

  restriction.fill((Restriction*) &_restriction);
  notifyConditionVector.fill(_notifyConditionVector);
  reference.set(_reference);
}



/* ****************************************************************************
*
* Subscription::entityIdInfoAdd - 
*
* FIXME P10: need to decide whether this function copies the EntityInfo or
*            just uses the pointer. For now, just using the pointer.
*/
void Subscription::entityIdInfoAdd(EntityInfo* entityIdInfoP)
{
  entityIdInfos.push_back(entityIdInfoP);
}



/* ****************************************************************************
*
* Subscription::attributeMatch - 
*/
bool Subscription::attributeMatch(const std::string& attributeName)
{
  LM_M(("KZ: attributeMatch: %d notifyConditions", notifyConditionVector.size()));

  for (unsigned int ncvIx = 0; ncvIx < notifyConditionVector.size(); ++ncvIx)
  {
    NotifyCondition* ncP = notifyConditionVector[ncvIx];

    if (strcasecmp(ncP->type.c_str(), "ONCHANGE") != 0)
    {
      LM_M(("KZ: attributeMatch: not ONCHANGE"));
      continue;
    }

    LM_M(("KZ: attributeMatch: %d condValues for notifyCondition %d", ncP->condValueList.size(), ncvIx));
    for (unsigned int cvIx = 0; cvIx < ncP->condValueList.size(); ++cvIx)
    {
      if (ncP->condValueList[cvIx] == attributeName)
      {
        LM_M(("KZ: Found matching attribute: '%s'", ncP->condValueList[cvIx].c_str()));
        return true;
      }
    }
  }

  LM_M(("KZ: No matching attribute for '%s'", attributeName.c_str()));
  return false;
}



/* ****************************************************************************
*
* match - 
*/
bool Subscription::match
(
  const std::string&  _tenant,
  const std::string&  _servicePath,
  const std::string&  id,
  const std::string&  type,
  const std::string&  attributeName
)
{
  if (_tenant != tenant)
  {
    LM_M(("KZ: Subscription::match: tenant not matching"));
    return false;
  }

  if (servicePathMatch(_servicePath) == false)
  {
    LM_M(("KZ: Subscription::match: servicePath not matching ('%s' vs '%s')", servicePath.c_str(), _servicePath.c_str()));
    return false;
  }

  //
  // If ONCHANGE and one of the attribute names in the scope vector
  // of the subscription has the same name as the incoming attribute. there is a match.
  //
  if (!attributeMatch(attributeName))
  {
    LM_M(("KZ: Subscription::match: attributes not matching (incoming: '%s')", attributeName.c_str()));
    return false;
  }

  for (unsigned int ix = 0; ix < entityIdInfos.size(); ++ix)
  {
    if (entityIdInfos[ix]->match(id, type))
    {
      LM_M(("KZ: Subscription::match: WE HAVE A MATCH !!!"));
      return true;
    }
  }

  LM_M(("KZ: Subscription::match: entityId not matching"));
  return false;
}



/* ****************************************************************************
*
* match - 
*/
bool Subscription::servicePathMatch
(
  const std::string&  _servicePath
)
{
  const char* spath = servicePath.c_str();

  //
  // No wildcard - exact match
  //
  if (spath[strlen(spath) - 1] != '#')
  {
    return (_servicePath == servicePath);
  }

  //
  // Wildcard - remove '#' and compare to the length of _servicePath.
  //            If equal upto the length of _servicePath, then it is a match
  //
  int len = strlen(servicePath.c_str()) - 1;

  if (strncmp(servicePath.c_str(), spath, len) == 0)
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* hasAttribute - 
*/
bool Subscription::hasAttribute(const std::string& attributeName)
{
  unsigned int ix;

  for (ix = 0; ix < attributes.size(); ++ix)
  {
    if (attributes[ix] == attributeName)
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* update - 
*/
void Subscription::update(UpdateContextSubscriptionRequest* ucsrP)
{
  // 1. Update expirationTime if 'duration' is set in UpdateContextSubscriptionRequest
  if (ucsrP->duration.get() != "")
  {
    int64_t _expirationTime = getCurrentTime() + ucsrP->duration.parse();
    expirationTime  = _expirationTime;
  }

  // 2. Update throttling if 'throttling' is set in UpdateContextSubscriptionRequest
  if (ucsrP->throttling.get() != "")
  {
    throttling = ucsrP->throttling.parse();
  }

  // 3. restriction
  // 4. notifyConditionVector
}



/* ****************************************************************************
*
* SubscriptionCache::SubscriptionCache - 
*/
SubscriptionCache::SubscriptionCache()
{
  dbName = "";
}



/* ****************************************************************************
*
* SubscriptionCache::SubscriptionCache - 
*/
SubscriptionCache::SubscriptionCache(std::string _dbName)
{
  dbName = _dbName;
}



/* ****************************************************************************
*
* semInit - 
*/
void SubscriptionCache::semInit(void)
{
  sem_init(&mutex, 0, 1); // Shared between threads, not taken initially
}



/* ****************************************************************************
*
* semTake - 
*/
void SubscriptionCache::semTake(void)
{
  struct timespec  startTime;
  struct timespec  endTime;
  struct timespec  diffTime;

  if (semTimeStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  sem_wait(&mutex);

  if (semTimeStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &endTime);
    clock_difftime(&endTime, &startTime, &diffTime);
    clock_addtime(&subCacheMutexTime, &diffTime);
  }
}


/* ****************************************************************************
*
* semGive - 
*/
void SubscriptionCache::semGive(void)
{
  sem_post(&mutex);
}



/* ****************************************************************************
*
* lookup - 
*/
void SubscriptionCache::lookup
(
  const std::string&           tenant,
  const std::string&           servicePath,
  const std::string&           id,
  const std::string&           type,
  const std::string&           attributeName,
  std::vector<Subscription*>*  subV
)
{
  LM_M(("KZ: Looking up a sub in a cache of %d subs (tenant='%s',spath='%s')", subs.size(), tenant.c_str(), servicePath.c_str()));

  for (unsigned int ix = 0; ix < subs.size(); ++ix)
  {
    if (subs[ix]->match(tenant, servicePath, id, type, attributeName))
    {
      subV->push_back(subs[ix]);
    }
  }

  ++noOfSubCacheLookups;
  LM_M(("KZ: returning a list of %d Subscriptions for the lookup", subV->size()));
}



/* ****************************************************************************
*
* lookupById - 
*/
Subscription* SubscriptionCache::lookupById
(
  const std::string&  tenant,
  const std::string&  servicePath,
  const std::string&  subId
)
{
  unsigned int ix;

  for (ix = 0; ix < subs.size(); ++ix)
  {
    if (subs[ix]->tenant != tenant)
    {
      continue;
    }

    if (subs[ix]->servicePathMatch(servicePath) == false)
    {
      continue;
    }

    if (subs[ix]->subscriptionId != subId)
    {
      continue;
    }

    return subs[ix];
  }

  return NULL;
}



/* ****************************************************************************
*
* subTreat - 
*/
static void subTreat(std::string tenant, BSONObj& bobj)
{
  BSONElement  idField = bobj.getField("_id");

  if (idField.eoo() == true)
  {
    LM_E(("Database Error (error retrieving _id field in doc: '%s')", bobj.toString().c_str()));
    return;
  }



  //
  // 01. Extract values from database object 'bobj'
  //

  std::string               subId         = idField.OID().toString();
  int64_t                   expiration    = bobj.getField("expiration").Long();
  std::string               reference     = bobj.getField("reference").String();
  int64_t                   throttling    = bobj.getField("throttling").Long();
  std::vector<BSONElement>  eVec          = bobj.getField("entities").Array();
  std::vector<BSONElement>  attrVec       = bobj.getField("attrs").Array();
  std::vector<BSONElement>  condVec       = bobj.getField("conditions").Array();
  std::string               formatString  = bobj.getField("format").String();
  std::string               servicePath   = bobj.getField("servicePath").String();
  Format                    format        = stringToFormat(formatString);

  std::vector<EntityInfo*> eiV;
  std::vector<std::string> attrV;
  Restriction              restriction;
  NotifyConditionVector    notifyConditionVector;

  //
  // 02. Pushing Entity-data names to EntityInfo Vector (eiV)
  //
  for (unsigned int ix = 0; ix < eVec.size(); ++ix)
  {
    BSONObj entity = eVec[ix].embeddedObject();

    if (!entity.hasField("id"))
    {
      LM_W(("Runtime Error (got a subscription without id)"));
      continue;
    }

    std::string id = entity.getStringField("id");
    
    if (!entity.hasField("isPattern"))
    {
      continue;
    }

    std::string isPattern = entity.getStringField("isPattern");
    if (isPattern != "true")
    {
      continue;
    }

    std::string  type = "";
    if (entity.hasField("type"))
    {
      type = entity.getStringField("type");
    }

    EntityInfo* eiP = new EntityInfo(id, type);
    eiV.push_back(eiP);
  }

  if (eiV.size() == 0)
  {
    return;
  }


  //
  // 03. Pushing attribute names to Attribute Vector (attrV)
  //
  for (unsigned int ix = 0; ix < attrVec.size(); ++ix)
  {
    std::string attributeName = attrVec[ix].String();

    attrV.push_back(attributeName);
  }


  //
  // 04. FIXME P4: Restriction not implemented
  //
  
  

  //
  // 05. Fill in notifyConditionVector from condVec
  //
  for (unsigned int ix = 0; ix < condVec.size(); ++ix)
  {
    BSONObj                   condition = condVec[ix].embeddedObject();
    std::string               condType;
    std::vector<BSONElement>  valueVec;

    condType = condition.getStringField("type");
    if (condType != "ONCHANGE")
    {
      continue;
    }

    NotifyCondition* ncP = new NotifyCondition();
    ncP->type = condType;

    valueVec = condition.getField("value").Array();
    for (unsigned int vIx = 0; vIx < valueVec.size(); ++vIx)
    {
      std::string condValue;

      condValue = valueVec[vIx].String();
      ncP->condValueList.push_back(condValue);
    }

    notifyConditionVector.push_back(ncP);
  }



  //
  // 06. Create Subscription and add it to the subscription-cache
  //
  Subscription* subP = new Subscription(tenant, servicePath, subId, eiV, attrV, throttling, expiration, restriction, notifyConditionVector, reference, format);
  subCache->insert(subP);
}



/* ****************************************************************************
*
* SubscriptionCache::init - 
*/
void SubscriptionCache::init(void)
{
  semInit();

  // FIXME P10: Get a list of all tenants and call subscriptionsTreat() for each of them
  std::vector<std::string> tenantV;

  tenantV.push_back(dbName);

  for (unsigned int ix = 0; ix < tenantV.size(); ++ix)
  {
    subscriptionsTreat(tenantV[ix], subTreat);
  }
}



/* ****************************************************************************
*
* SubscriptionCache::insert - 
*/
void SubscriptionCache::insert(Subscription* subP)
{
  if (subP->entityIdInfos.size() == 0)
  {
    LM_E(("Runtime Error (no entity path for subscription - not inserted in subscription cache)"));
    return;
  }

  LM_M(("KZ: Inserting a subscription in the cache"));
  LM_M(("KZ: -------------------------------------"));
  LM_M(("KZ: inserting a subscription in the cache: tenant      = '%s'", subP->tenant.c_str()));
  LM_M(("KZ: inserting a subscription in the cache: servicePath = '%s'", subP->servicePath.c_str()));
  LM_M(("KZ: inserting a subscription in the cache: entityIdInfos: %d",  subP->entityIdInfos.size()));

  for (unsigned int eiIx = 0; eiIx < subP->entityIdInfos.size(); ++eiIx)
  {
    LM_M(("KZ: inserting a subscription in the cache: entityIdInfo[%d]->type: '%s'", eiIx, subP->entityIdInfos[eiIx]->entityType.c_str()));
  }

  for (unsigned int aIx = 0; aIx < subP->attributes.size(); ++aIx)
  {
    LM_M(("KZ: inserting a subscription in the cache: attributes[%d] = '%s'", aIx, subP->attributes[aIx].c_str()));
  }
  if (subP->attributes.size() == 0)
  {
    LM_M(("KZ: inserting a subscription in the cache: ALL attributes"));
  }

  LM_M(("KZ: -------------------------------------"));

  semTake();
  subs.push_back(subP);
  semGive();

  ++noOfSubCacheEntries;
}



/* ****************************************************************************
*
* SubscriptionCache::remove - 
*/
int SubscriptionCache::remove(Subscription* subP)
{
  unsigned int ix;

  for (ix = 0; ix < subs.size(); ++ix)
  {
    if (subs[ix] == subP)
    {
      semTake();

      free(subP);
      subs.erase(subs.begin() + ix);
      
      semGive();

      ++noOfSubCacheRemovals;
      --noOfSubCacheEntries;
      return 0;
    }
  }

  ++noOfSubCacheRemovalFailures;
  return -1;
}



/* ****************************************************************************
*
* SubscriptionCache::remove - 
*/
int SubscriptionCache::remove
(
  const std::string& tenant,
  const std::string& servicePath,
  const std::string& subId
)
{
  Subscription* subP = lookupById(tenant, servicePath, subId);

  if (subP == NULL)
  {
    return -1;
  }

  return remove(subP);
}



/* ****************************************************************************
*
* SubscriptionCache::refresh - 
*/
int SubscriptionCache::refresh(void)
{
  LM_W(("NOT IMPLEMENMTED"));
  return 0;
}

}
