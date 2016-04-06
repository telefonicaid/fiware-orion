#ifndef SRC_LIB_CACHE_SUBCACHE_H_
#define SRC_LIB_CACHE_SUBCACHE_H_

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
#include "apiTypesV2/SubscriptionExpression.h"

using namespace mongo;


/* ****************************************************************************
*
* SubCacheState - 
*/
typedef enum SubCacheState
{
  ScsIdle,
  ScsSynchronizing
} SubCacheState;



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
struct EntityInfo
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
};



/* ****************************************************************************
*
* CachedSubscription - 
*/
struct CachedSubscription
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
  std::string                 status;
  int64_t                     count;
  Format                      notifyFormat;
  char*                       reference;
  SubscriptionExpression      expression;
  struct CachedSubscription*  next;
};



/* ****************************************************************************
*
* subCacheActive - 
*/
extern bool                    subCacheActive;
extern volatile SubCacheState  subCacheState;



/* ****************************************************************************
*
* subCacheInit - 
*/
extern void subCacheInit(void);



/* ****************************************************************************
*
* subCacheStart - 
*/
extern void subCacheStart(void);



/* ****************************************************************************
*
* subCacheDestroy - 
*/
extern void subCacheDestroy(void);



/* ****************************************************************************
*
* subCacheItemDestroy - 
*/
extern void subCacheItemDestroy(CachedSubscription* cSubP);



/* ****************************************************************************
*
* subCacheItems - 
*/
extern int subCacheItems(void);



/* ****************************************************************************
*
* subCachePresent - 
*/
extern void subCachePresent(const char* title);



/* ****************************************************************************
*
* subCacheItemInsert - 
*/
extern void subCacheItemInsert
(
  const char*               tenant,
  const char*               servicePath,
  SubscribeContextRequest*  scrP,
  const char*               subscriptionId,
  int64_t                   expiration,
  int64_t                   throttling,
  Format                    notifyFormat,
  bool                      notificationDone,
  int64_t                   lastNotificationTime,
  const std::string&        status,
  const std::string&        q,
  const std::string&        geometry,
  const std::string&        coords,
  const std::string&        georel
);



/* ****************************************************************************
*
* subCacheItemInsert - 
*/
extern void subCacheItemInsert(CachedSubscription* cSubP);



/* ****************************************************************************
*
* subCacheItemLookup - 
*/
extern CachedSubscription* subCacheItemLookup(const char* tenant, const char* subscriptionId);



/* ****************************************************************************
*
* subCacheItemRemove - 
*/
extern int subCacheItemRemove(CachedSubscription* cSubP);



/* ****************************************************************************
*
* subCacheRefresh - 
*/
extern void subCacheRefresh(void);



/* ****************************************************************************
*
* subCacheSync - 
*/
extern void subCacheSync(void);



/* ****************************************************************************
*
* subCacheMatch - 
*/
extern void subCacheMatch
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
* subCacheMatch - 
*/
extern void subCacheMatch
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
* subCacheStatisticsGet - 
*/
extern void subCacheStatisticsGet(int* refreshes, int* inserts, int* removes, int* updates, int* items, char* list, int listSize);



/* ****************************************************************************
*
* subCacheUpdateStatisticsIncrement - 
*/
extern void subCacheUpdateStatisticsIncrement(void);



/* ****************************************************************************
*
* subCacheStatisticsReset - 
*/
extern void subCacheStatisticsReset(const char* by);

#endif  // SRC_LIB_CACHE_SUBCACHE_H_
