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
#include "mongoBackend/MongoGlobal.h"
#include "cache/SubscriptionCache.h"
#include "cache/subCache.h"



namespace orion
{


/* ****************************************************************************
*
* subCacheMutexTime - 
*/
static struct timespec subCacheMutexTime = { 0, 0 };



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
* SubscriptionCache::SubscriptionCache - 
*/
SubscriptionCache::SubscriptionCache()
{
  dbPrefix = "";
}



/* ****************************************************************************
*
* SubscriptionCache::SubscriptionCache - 
*/
SubscriptionCache::SubscriptionCache(std::string _dbPrefix)
{
  dbPrefix = _dbPrefix;
}



/* ****************************************************************************
*
* SubscriptionCache::semInit - 
*/
void SubscriptionCache::semInit(void)
{
  sem_init(&mutex, 0, 1); // Shared between threads, not taken initially
}



/* ****************************************************************************
*
* SubscriptionCache::semTake - 
*/
void SubscriptionCache::semTake(void)
{
  if (pthread_self() == mutexOwner)
  {
    return;
  }

  struct timespec  startTime;
  struct timespec  endTime;
  struct timespec  diffTime;

  if (semTimeStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  sem_wait(&mutex);
  mutexOwner = pthread_self();

  if (semTimeStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &endTime);
    clock_difftime(&endTime, &startTime, &diffTime);
    clock_addtime(&subCacheMutexTime, &diffTime);
  }
}



/* ****************************************************************************
*
* SubscriptionCache::semGive - 
*/
void SubscriptionCache::semGive(void)
{
  sem_post(&mutex);
  mutexOwner = 0;
}



/* ****************************************************************************
*
* SubscriptionCache::lookup - 
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
  for (unsigned int ix = 0; ix < subs.size(); ++ix)
  {
    if (subs[ix]->match(tenant, servicePath, id, type, attributeName))
    {
      subV->push_back(subs[ix]);
    }
  }

  ++noOfSubCacheLookups;
}



/* ****************************************************************************
*
* SubscriptionCache::lookupById - 
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
* SubscriptionCache::lookupById - 
*/
Subscription* SubscriptionCache::lookupById(const std::string&  subId)
{
  unsigned int  ix;
  const char*   id = subId.c_str();

  for (ix = 0; ix < subs.size(); ++ix)
  {
    if (subs[ix]->subscriptionId != id)
    {
      continue;
    }

    return subs[ix];
  }

  return NULL;
}



/* ****************************************************************************
*
* subToCache - 
*/
static void subToCache(std::string tenant, BSONObj& bobj)
{
  subCache->insert(tenant, bobj);
}



/* ****************************************************************************
*
* SubscriptionCache::init - 
*/
void SubscriptionCache::init(void)
{
  semInit();

  semTake();
  fillFromDb();
  semGive();
}



/* ****************************************************************************
*
* SubscriptionCache::fillFromDb - 
*/
void SubscriptionCache::fillFromDb(void)
{
  std::vector<std::string> databases;

  getOrionDatabases(databases);

  //
  // Add the 'default tenant'
  //
  databases.push_back(dbPrefix);

  for (unsigned int ix = 0; ix < databases.size(); ++ix)
  {
    subscriptionsTreat(databases[ix], subToCache);
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
    LM_E(("Runtime Error (no entity for subscription - not inserted in subscription cache)"));
    return;
  }

  subs.push_back(subP);

  ++noOfSubCacheEntries;
}



/* ****************************************************************************
*
* SubscriptionCache::insert - 
*/
void SubscriptionCache::insert(const std::string& tenant, BSONObj bobj)
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
  std::string               subId             = idField.OID().toString();
  int64_t                   expiration        = bobj.hasField(CSUB_EXPIRATION)? bobj.getField(CSUB_EXPIRATION).Long() : 0;
  std::string               reference         = bobj.getField(CSUB_REFERENCE).String();
  int64_t                   throttling        = bobj.hasField(CSUB_THROTTLING)? bobj.getField(CSUB_THROTTLING).Long() : -1;
  std::vector<BSONElement>  eVec              = bobj.getField(CSUB_ENTITIES).Array();
  std::vector<BSONElement>  attrVec           = bobj.getField(CSUB_ATTRS).Array();
  std::vector<BSONElement>  condVec           = bobj.getField(CSUB_CONDITIONS).Array();
  std::string               formatString      = bobj.hasField(CSUB_FORMAT)? bobj.getField(CSUB_FORMAT).String() : "XML";
  std::string               servicePath       = bobj.hasField(CSUB_SERVICE_PATH)? bobj.getField(CSUB_SERVICE_PATH).String() : "/";
  Format                    format            = stringToFormat(formatString);
  int                       lastNotification  = bobj.hasField(CSUB_LASTNOTIFICATION)? bobj.getField(CSUB_LASTNOTIFICATION).Int() : 0;
  std::vector<EntityInfo*>  eiV;
  std::vector<std::string>  attrV;
  Restriction               restriction;
  NotifyConditionVector     notifyConditionVector;


  //
  // 02. Push Entity-data names to EntityInfo Vector (eiV)
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

    std::string  type = "";
    if (entity.hasField(CSUB_ENTITY_TYPE))
    {
      type = entity.getStringField(CSUB_ENTITY_TYPE);
    }

    EntityInfo* eiP = new EntityInfo(id, type);
    eiV.push_back(eiP);
  }

  if (eiV.size() == 0)
  {
    return;
  }


  //
  // 03. Push attribute names to Attribute Vector (attrV)
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

    notifyConditionVector.push_back(ncP);
  }

  if (notifyConditionVector.size() == 0)
  {
    for (unsigned int ix = 0; ix < eiV.size(); ++ix)
    {
      delete(eiV[ix]);
    }
    eiV.clear();

    restriction.release();

    return;
  }


  //
  // 06. Create Subscription and add it to the subscription-cache
  //
  Subscription* subP = new Subscription(tenant,
                                        servicePath,
                                        subId,
                                        eiV,
                                        attrV,
                                        throttling,
                                        expiration,
                                        restriction,
                                        notifyConditionVector,
                                        reference,
                                        lastNotification,
                                        format);
  
  subCache->insert(subP);

  notifyConditionVector.release();  // Subscription constructor makes a copy
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
      subP->release();
      delete(subP);

      subs.erase(subs.begin() + ix);
      
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
* SubscriptionCache::release - 
*/
void SubscriptionCache::release(void)
{
  for (unsigned int sIx = 0; sIx < subs.size(); ++sIx)
  {
    subs[sIx]->release();
    delete(subs[sIx]);
  }

  subs.clear();
}



/* ****************************************************************************
*
* SubscriptionCache::refresh - 
*/
int SubscriptionCache::refresh(void)
{
  release();
  fillFromDb();

  return 0;
}



/* ****************************************************************************
*
* SubscriptionCache::present - 
*/
void SubscriptionCache::present(const std::string& prefix)
{
  LM_F(("%sSubscription Cache with %d subscriptions", prefix.c_str(), subs.size()));

  for (unsigned int ix = 0; ix < subs.size(); ++ix)
  {
    LM_F(("%s  Subscription %d:", prefix.c_str(), ix));
    subs[ix]->present(prefix + "    ");
    LM_F(("%s  ------------------------------------------------", prefix.c_str()));
  }
}

}
