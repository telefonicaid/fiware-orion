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
#include <regex.h>
#include <string>
#include <vector>

#include "mongo/client/dbclient.h"

#include "common/RenderFormat.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/StringList.h"
#include "orionld/types/OrionldAlteration.h"                 // OrionldAlterationTypes
#include "orionld/context/OrionldContext.h"                  // OrionldContext
#include "apiTypesV2/HttpInfo.h"
#include "apiTypesV2/SubscriptionExpression.h"
#include "apiTypesV2/Subscription.h"
#include "orionld/q/QNode.h"                                 // QNode



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
* o entityType           string containing the type of the Entity
*
*/
struct EntityInfo
{
  std::string   entityId;
  bool          isPattern;
  regex_t       entityIdPattern;
  bool          entityIdPatternToBeFreed;

  std::string   entityType;
  bool          isTypePattern;
  regex_t       entityTypePattern;
  bool          entityTypePatternToBeFreed;


  EntityInfo() {}
  EntityInfo(const std::string& _entityId, const std::string& _entityType, const std::string& _isPattern,
             bool _isTypePattern);
  ~EntityInfo() { release(); }

  bool          match(const std::string& idPattern, const std::string& type);
  void          release(void);
};



/* ****************************************************************************
*
* CachedSubscription - move to cache/CachedSubscription.h
*/
struct CachedSubscription
{
  char*                       subscriptionId;
  char*                       description;
  std::string                 name;

  char*                       url;       // Copy of httpInfo.url (parsed and destroyed) - allocated and must be freed
  char*                       protocol;  // pointing to 'protocol' part of 'url'
  char*                       ip;        // pointing to 'ip' part of 'url'
  unsigned short              port;      // port, as parsed from 'url'
  char*                       rest;      // pointing to 'rest' part of 'url'

  std::vector<EntityInfo*>    entityIdInfos;
  std::vector<std::string>    attributes;
  std::vector<std::string>    metadata;
  std::vector<std::string>    notifyConditionV;
  char*                       tenant;
  char*                       servicePath;
  bool                        triggers[OrionldAlterationTypes];
  double                      throttling;
  double                      expirationTime;
  std::string                 ldContext;
  OrionldContext*             contextP;
  std::string                 lang;
  RenderFormat                renderFormat;
  SubscriptionExpression      expression;
  bool                        blacklist;
  ngsiv2::HttpInfo            httpInfo;
  QNode*                      qP;
  char*                       qText;  // Note that NGSIv2/mongoBackend q/mq are inside SubscriptionExpression
  KjNode*                     geoCoordinatesP;

  bool                        isActive;
  std::string                 status;
  int64_t                     count;
  double                      lastNotificationTime;  // timestamp of last notification attempt
  double                      lastFailure;           // timestamp of last notification failure
  double                      lastSuccess;           // timestamp of last successful notification
  int                         consecutiveErrors;     // Not in DB
  char                        lastErrorReason[128];

  double                      createdAt;
  double                      modifiedAt;

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
* subCacheHeadGet -
*/
extern CachedSubscription* subCacheHeadGet(void);



/* ****************************************************************************
*
* subCacheInit -
*/
extern void subCacheInit(bool multitenant = false);



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
* subCacheDisable -
*
*/
#ifdef UNIT_TEST
void subCacheDisable(void);
#endif



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
* subCacheItemInsert -
*/
extern void subCacheItemInsert
(
  const char*                        tenant,
  const char*                        servicePath,
  const ngsiv2::HttpInfo&            httpInfo,
  const std::vector<ngsiv2::EntID>&  entities,
  const std::vector<std::string>&    attributes,
  const std::vector<std::string>&    metadata,
  const std::vector<std::string>&    conditionAttrs,
  const char*                        subscriptionId,
  double                             expiration,
  double                             throttling,
  RenderFormat                       renderFormat,
  bool                               notificationDone,
  double                             lastNotificationTime,
  double                             lastNotificationSuccessTime,
  double                             lastNotificationFailureTime,
  StringFilter*                      stringFilterP,
  StringFilter*                      mdStringFilterP,
  const std::string&                 status,
#ifdef ORIONLD
  const std::string&                 name,
  const std::string&                 ldContext,
  const std::string&                 lang,
  const char*                        mqttUserName,
  const char*                        mqttPassword,
  const char*                        mqttVersion,
  int                                mqttQoS,
#endif
  const std::string&                 q,
  const std::string&                 geometry,
  const std::string&                 coords,
  const std::string&                 georel,
#ifdef ORIONLD
  const std::string&                 geoproperty,
#endif
  bool                               blacklist
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
extern int subCacheMatch
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
extern int subCacheMatch
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
extern void subCacheStatisticsGet
(
  int*   refreshes,
  int*   inserts,
  int*   removes,
  int*   updates,
  int*   items,
  char*  list,
  int    listSize
);



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



/* ****************************************************************************
*
* subCacheItemNotificationErrorStatus -
*/
extern void subCacheItemNotificationErrorStatus
(
  const std::string&  tenant,
  const std::string&  subscriptionId,
  int                 errors,
  bool                ngsild
);



/* ****************************************************************************
*
* tenantMatch -
*/
extern bool tenantMatch(const char* tenant1, const char* tenant2);



/* ****************************************************************************
*
* subscriptionFailure -
*/
extern void subscriptionFailure(CachedSubscription* subP, const char* errorReason, double timestamp);



/* ****************************************************************************
*
* subscriptionSuccess -
*/
extern void subscriptionSuccess(CachedSubscription* subP, double timestamp);

#endif  // SRC_LIB_CACHE_SUBCACHE_H_
