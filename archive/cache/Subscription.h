#ifndef SRC_LIB_CACHE_SUBSCRIPTION_H_
#define SRC_LIB_CACHE_SUBSCRIPTION_H_

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

#include "mongo/client/dbclient.h"

#include "common/Format.h"
#include "ngsi/Restriction.h"
#include "ngsi/Reference.h"
#include "ngsi/NotifyConditionVector.h"
#include "cache/EntityInfo.h"

struct SubscribeContextRequest;
struct UpdateContextSubscriptionRequest;

using namespace mongo;

namespace orion
{


/* ****************************************************************************
*
* Subscription -
*
* The class fields:
* -------------------------------------------------------------------------------
* o subscriptionId         the subscription ID from the database
* o entityIdInfos          list of EntityInfo
* o attributes             list of strings of attribute names
* o throttling             throttling time in seconds
* o expirationTime         expiration time expressed as Unix seconds since epoch
* o restriction            Restriction (OMA NGSI type)
* o notifyConditionVector  list of NotifyCondition (OMA NGSI type)
* o Reference              Info on the receiver of the notification
*
*/
class Subscription
{
 public:
  std::string               tenant;
  std::string               servicePath;
  std::string               subscriptionId;
  std::vector<EntityInfo*>  entityIdInfos;
  std::vector<std::string>  attributes;
  int64_t                   throttling;
  int64_t                   expirationTime;
  Restriction               restriction;
  NotifyConditionVector     notifyConditionVector;
  Reference                 reference;

  int                       lastNotificationTime;
  int                       pendingNotifications;
  Format                    format;

  Subscription();
  Subscription(const std::string& _tenant, const std::string& _servicePath, const std::string& _subscriptionId, Format _format);

  Subscription(const std::string&        _tenant,
               const std::string&        _servicePath,
               SubscribeContextRequest*  scrP,
               std::string               _subscriptionId,
               int64_t                   _expiration,
               Format                    _format);

  Subscription(const std::string&               _tenant,
               const std::string&               _servicePath,
               const std::string&               _subscriptionId,
               const std::vector<EntityInfo*>&  _entityIdInfos,
               const std::vector<std::string>&  _attributes,
               int64_t                          _throttling,
               int64_t                          _expirationTime,
               const Restriction&               _restriction,
               NotifyConditionVector&           _notifyConditionVector,
               const std::string&               _reference,
               int                              _lastNotificationTime,
               Format                           _format);

  void entityIdInfoAdd(EntityInfo* entityIdInfoP);
  bool match(const std::string&  _tenant,
             const std::string&  _servicePath,
             const std::string&  idPattern,
             const std::string&  type,
             const std::string&  attributeName);
  bool servicePathMatch(const std::string&  _servicePath);
  bool attributeMatch(const std::string& attributeName);
  bool hasAttribute(const std::string&attributeName);
  void present(const std::string& prefix);
  void release(void);
};

}  // namespace orion

#endif  // SRC_LIB_CACHE_SUBSCRIPTION_H_
