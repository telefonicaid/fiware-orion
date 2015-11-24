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

#include "mongo/client/dbclient.h"

#include "ngsi/NotifyConditionVector.h"
#include "ngsi10/SubscribeContextRequest.h"

using namespace mongo;



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
  bool          isPattern;
  regex_t       entityIdPattern;
  std::string   entityId;
  std::string   entityType;
  bool          entityIdPatternToBeFreed;

  EntityInfo() {}
  EntityInfo(const std::string& _entityId, const std::string& _entityType, const std::string& _isPattern);
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
  std::vector<EntityInfo*>    entityIdInfos;
  std::vector<std::string>    attributes;
  NotifyConditionVector       notifyConditionVector;
  char*                       tenant;
  char*                       servicePath;
  char*                       subscriptionId;
  int64_t                     throttling;
  int64_t                     expirationTime;
  int64_t                     lastNotificationTime;
  int                         pendingNotifications;
  Format                      notifyFormat;
  char*                       reference;
  struct CachedSubscription*  next;
} CachedSubscription;



/* ****************************************************************************
*
* mongoSubCacheInit - 
*/
extern void mongoSubCacheInit(void);



/* ****************************************************************************
*
* mongoSubCacheStart - 
*/
extern void mongoSubCacheStart(void);



/* ****************************************************************************
*
* mongoSubCacheDestroy - 
*/
extern void mongoSubCacheDestroy(void);



/* ****************************************************************************
*
* mongoSubCacheItems - 
*/
extern int mongoSubCacheItems(void);



/* ****************************************************************************
*
* mongoSubCachePresent - 
*/
extern void mongoSubCachePresent(const char* title);



/* ****************************************************************************
*
* mongoSubCacheItemInsert - 
*/
extern int mongoSubCacheItemInsert(const char* tenant, const BSONObj& sub);



/* ****************************************************************************
*
* mongoSubCacheItemInsert - 
*/
extern void mongoSubCacheItemInsert
(
  const char*               tenant,
  const char*               servicePath,
  SubscribeContextRequest*  scrP,
  const char*               subscriptionId,
  int64_t                   expiration,
  int64_t                   throttling,
  Format                    notifyFormat
);



/* ****************************************************************************
*
* mongoSubCacheItemInsert - 
*/
extern int mongoSubCacheItemInsert(const char* tenant, const BSONObj& sub, const char* subscriptionId, const char* servicePath, int lastNotificationTime, long long expirationTime);



/* ****************************************************************************
*
* mongoSubCacheItemLookup - 
*/
extern CachedSubscription* mongoSubCacheItemLookup(const char* tenant, const char* subscriptionId);



/* ****************************************************************************
*
* mongoSubCacheItemRemove - 
*/
extern int mongoSubCacheItemRemove(CachedSubscription* cSubP);



/* ****************************************************************************
*
* mongoSubCacheRefresh - 
*/
extern void mongoSubCacheRefresh(void);



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
  const std::vector<std::string>&    attrV,
  std::vector<CachedSubscription*>*  subVecP
);



/* ****************************************************************************
*
* mongoSubCacheStatisticsGet - 
*/
extern void mongoSubCacheStatisticsGet(int* refreshes, int* inserts, int* removes, int* updates, int* items, char* list, int listSize);



/* ****************************************************************************
*
* mongoSubCacheUpdateStatisticsIncrement - 
*/
extern void mongoSubCacheUpdateStatisticsIncrement(void);



/* ****************************************************************************
*
* mongoSubCacheStatisticsReset - 
*/
extern void mongoSubCacheStatisticsReset(const char* by);

#endif  // SRC_LIB_MONGOBACKEND_MONGOSUBCACHE_H_
