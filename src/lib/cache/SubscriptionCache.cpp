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
void subscriptionCacheInit(void)
{
  LM_M(("KZ: creating Subscription Cache"));
  subCache = new SubscriptionCache();
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
bool EntityInfo::match(const std::string& id, const std::string& type)
{
  LM_M(("KZ: matching against idPattern '%s' / type '%s'", id.c_str(), type.c_str()));

  //
  // If type non-empty - perfect match is mandatory
  // If type is empty - always matches
  //
  if ((type != "") && (entityType != type))
  {
    return false;
  }

  // REGEX comparison this->entityIdPattern VS id
  return regexec(&entityIdPattern, id.c_str(), 0, NULL, 0) == 0;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription()
{
  subscriptionId = "";
  throttling     = "";
  expirationTime = 0;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription(const std::string& _subscriptionId)
{
  subscriptionId = _subscriptionId;
  throttling     = "";
  expirationTime = 0;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription(SubscribeContextRequest* scrP, std::string _subscriptionId, int64_t _expiration)
{
  unsigned int ix;

  //
  // 1. Get Subscription Id
  //
  subscriptionId = _subscriptionId;


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
  throttling = scrP->throttling.get().c_str();


  //
  // 5. expirationTime
  //
  expirationTime = _expiration;


  //
  // 6. restriction
  // 7. notifyConditionVector
  // 8. reference
  //
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription
(
  const std::string&               _subscriptionId,
  const std::vector<EntityInfo*>&  _entityIdInfos,
  const std::vector<std::string>&  _attributes,
  const std::string&               _throttling,
  int64_t                          _expirationTime,
  const Restriction&               _restriction,
  const NotifyConditionVector&     _notifyConditionVector,
  const std::string&               _reference
)
{
  unsigned int ix;

  subscriptionId = _subscriptionId;

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

  restriction.fill(_restriction);
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
* match - 
*/
bool Subscription::match(const std::string& id, const std::string& type, const std::string& attributeName)
{
  LM_M(("KZ: matching against id '%s' / type '%s' / attr '%s'", id.c_str(), type.c_str(), attributeName.c_str()));

  if (!hasAttribute(attributeName))
  {
    LM_M(("KZ: not even attribute name was ok ..."));
    return false;
  }

  LM_M(("KZ: attribute OK"));
  unsigned int ix;

  LM_M(("KZ: searching an entity in vector of %d entity patterns", entityIdInfos.size()));

  for (ix = 0; ix < entityIdInfos.size(); ++ix)
  {
    LM_M(("KZ: calling match method for an entityIdInfo"));
    if (entityIdInfos[ix]->match(id, type))
    {
      LM_M(("KZ: match!"));
      return true;
    }
  }

  LM_M(("KZ: no match ..."));
  return false;
}



/* ****************************************************************************
*
* hasAttribute - 
*/
bool Subscription::hasAttribute(const std::string& attributeName)
{
  unsigned int ix;

  LM_M(("KZ: looking for attribute '%s' in a vector of %d attributes", attributeName.c_str(), attributes.size()));

  for (ix = 0; ix < attributes.size(); ++ix)
  {
    LM_M(("KZ: Comparing '%s' to attribute %d: '%s'", attributeName.c_str(), ix, attributes[ix].c_str()));
    if (attributes[ix] == attributeName)
    {
      LM_M(("KZ: got a match in attribute name"));
      return true;
    }
  }

  LM_M(("KZ: no match in attribute name"));
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
    throttling = ucsrP->throttling.get();
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
  semInit();
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
  const std::string&           id,
  const std::string&           type,
  const std::string&           attributeName,
  std::vector<Subscription*>*  subV
)
{
  for (unsigned int ix = 0; ix < subs.size(); ++ix)
  {
    if (subs[ix]->match(id, type, attributeName))
    {
      subV->push_back(subs[ix]);
    }
  }

  ++noOfSubCacheLookups;
}



/* ****************************************************************************
*
* lookupById - 
*/
Subscription* SubscriptionCache::lookupById(const std::string& subId)
{
  unsigned int ix;

  for (ix = 0; ix < subs.size(); ++ix)
  {
    if (subs[ix]->subscriptionId == subId)
    {
      return subs[ix];
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* SubscriptionCache::insert - 
*/
void SubscriptionCache::insert(Subscription* subP)
{
  LM_M(("KZ: Inserting subscription '%s' in cache", subP->subscriptionId.c_str()));

  if (subP->entityIdInfos.size() == 0)
  {
    LM_M(("KZ: no entity id patterns - no insert performed"));
    return;
  }

  semTake();
  subs.push_back(subP);
  semGive();
  LM_M(("KZ: Subscription cache size: %d", subs.size()));
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
      LM_M(("KZ: Really removing subscription '%s' from cache (cache size: %d)", subP->subscriptionId.c_str(), subs.size()));
      semTake();

      free(subP);
      subs.erase(subs.begin() + ix);
      
      semGive();

      ++noOfSubCacheRemovals;
      LM_M(("KZ: Really removed subscription '%s' from cache (cache size: %d)", subP->subscriptionId.c_str(), subs.size()));
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
int SubscriptionCache::remove(const std::string& subId)
{
  Subscription* subP = lookupById(subId);

  LM_M(("KZ: Removing subscription '%s' from cache", subId.c_str()));

  if (subP == NULL)
  {
    LM_M(("KZ: subscription '%s' not found", subId.c_str()));
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
