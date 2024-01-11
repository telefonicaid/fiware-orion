#ifndef SRC_LIB_CACHE_CACHEDSUBSCRIPTION_H_
#define SRC_LIB_CACHE_CACHEDSUBSCRIPTION_H_

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

#include "common/RenderFormat.h"
#include "apiTypesV2/HttpInfo.h"                             // HttpInfo
#include "apiTypesV2/SubscriptionExpression.h"

#include "orionld/types/QNode.h"                             // QNode
#include "orionld/types/Protocol.h"                          // Protocol
#include "orionld/types/OrionldAlteration.h"                 // OrionldAlterationTypes
#include "orionld/types/OrionldTenant.h"                     // OrionldTenant
#include "orionld/types/OrionldContext.h"                    // OrionldContext



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
* CachedSubscription -
*/
struct CachedSubscription
{
  char*                       subscriptionId;
  char*                       description;
  std::string                 name;

  char*                       url;             // Copy of httpInfo.url (parsed and destroyed) - allocated and must be freed
  char*                       protocolString;  // pointing to 'protocol' part of 'url'
  char*                       ip;              // pointing to 'ip' part of 'url'
  unsigned short              port;            // port, as parsed from 'url'
  char*                       rest;            // pointing to 'rest' part of 'url'
  Protocol                    protocol;

  std::vector<EntityInfo*>    entityIdInfos;
  std::vector<std::string>    attributes;
  std::vector<std::string>    metadata;
  std::vector<std::string>    notifyConditionV;
  char*                       tenant;
  OrionldTenant*              tenantP;
  char*                       servicePath;
  bool                        triggers[OrionldAlterationTypes];
  double                      throttling;
  double                      expirationTime;
  std::string                 ldContext;
  OrionldContext*             contextP;
  std::string                 lang;
  OrionldRenderFormat         renderFormat;
  SubscriptionExpression      expression;
  bool                        blacklist;
  ngsiv2::HttpInfo            httpInfo;
  QNode*                      qP;
  char*                       qText;  // Note that NGSIv2/mongoBackend q/mq are inside SubscriptionExpression
  KjNode*                     geoCoordinatesP;
  bool                        showChanges;
  bool                        sysAttrs;

  bool                        isActive;
  std::string                 status;
  int64_t                     count;                 // delta count - since last sub cache refresh
  int64_t                     dbCount;               // count taken from the database
  int64_t                     failures;
  int64_t                     dbFailures;
  int                         consecutiveErrors;     // Not in DB, no need

  double                      lastNotificationTime;  // timestamp of last notification attempt
  double                      lastFailure;           // timestamp of last notification failure
  double                      lastSuccess;           // timestamp of last successful notification
  char                        lastErrorReason[128];  // Not in DB
  uint32_t                    dirty;

  double                      createdAt;
  double                      modifiedAt;

  struct CachedSubscription*  next;
  bool                        inDB;                  // Used by mongocSubCachePopulateByTenant to find subs that have been removed
};

#endif  // SRC_LIB_CACHE_CACHEDSUBSCRIPTION_H_
