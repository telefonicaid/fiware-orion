#ifndef SRC_LIB_MONGOBACKEND_MONGOSUBCACHE_H_
#define SRC_LIB_MONGOBACKEND_MONGOSUBCACHE_H_

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
#include <vector>
#include <regex.h>

#include "ngsi/NotifyConditionVector.h"



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
  NotifyConditionVector       notifyConditionVector;
  struct CachedSubscription*  next;
} CachedSubscription;



/* ****************************************************************************
*
* mongoSubCacheInit - 
*/
extern void mongoSubCacheInit(void);



/* ****************************************************************************
*
* mongoSubCacheSemTake - 
*/
extern void mongoSubCacheSemTake(const char* who);



/* ****************************************************************************
*
* mongoSubCacheSemGive - 
*/
extern void mongoSubCacheSemGive(const char* who);



/* ****************************************************************************
*
* mongoSubCacheRefresh - 
*/
extern void mongoSubCacheRefresh(void);



/* ****************************************************************************
*
* mongoSubCacheDestroy - 
*/
extern void mongoSubCacheDestroy(void);



/* ****************************************************************************
*
* mongoSubCacheMatch - 
*/
extern void mongoSubCacheMatch
(
  const char*                        tenant,
  const char*                        servicePath,
  const char*                        entityId,
  const char*                        entityType,
  const char*                        attr,
  std::vector<CachedSubscription*>*  subVecP
);

#endif  // SRC_LIB_MONGOBACKEND_MONGOSUBCACHE_H_
