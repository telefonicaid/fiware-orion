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

#include "logMsg/logMsg.h"

#include "ngsi/Restriction.h"
#include "ngsi/Reference.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/EntityId.h"
#include "cache/SubscriptionCache.h"

namespace orion
{

/* ****************************************************************************
*
* match - 
*/
bool EntityInfo::match(const std::string& idPattern, const std::string& type)
{
  if ((type != "") && (entityType != type))
  {
    return false;
  }

  // REGEX comparison this->entityIdPattern VS idPattern
  if (regcomp(&entityIdPattern, idPattern.c_str(), 0) != 0)
  {
    LM_W(("Bad Input (error compiling regex: '%s')", idPattern.c_str()));
    return false;
  }

  return regexec(&entityIdPattern, idPattern.c_str(), 0, NULL, 0) == 0;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription()
{
  subscriptionId = "";
  throttling     = 0;
  expirationTime = 0;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription(const std::string& _subscriptionId)
{
  subscriptionId = _subscriptionId;
  throttling     = 0;
  expirationTime = 0;
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
  int64_t                          _throttling,
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
  if (!hasAttribute(attributeName))
  {
    return false;
  }

  unsigned int ix;

  for (ix = 0; ix < entityIdInfos.size(); ++ix)
  {
    if (entityIdInfos[ix]->match(id, type))
    {
      return true;
    }
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
* SubscriptionCache::SubscriptionCache - 
*/
SubscriptionCache::SubscriptionCache()
{
  sem_init(&mutex, 0, 1); // Shared between threads, not taken initially
}



/* ****************************************************************************
*
* lookup - 
*/
Subscription* SubscriptionCache::lookup(const std::string& id, const std::string& type, const std::string& attributeName)
{
  unsigned int ix;

  for (ix = 0; ix < subs.size(); ++ix)
  {
    if (subs[ix]->match(id, type, attributeName))
    {
      return subs[ix];
    }
  }

  return NULL;
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
  sem_wait(&mutex);
  subs.push_back(subP);
  sem_post(&mutex);
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
      sem_wait(&mutex);

      free(subP);
      subs.erase(subs.begin() + ix);
      
      sem_post(&mutex);

      return 0;
    }
  }

  return -1;
}



/* ****************************************************************************
*
* SubscriptionCache::remove - 
*/
int SubscriptionCache::remove(const std::string& subId)
{
  Subscription* subP = lookupById(subId);

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
