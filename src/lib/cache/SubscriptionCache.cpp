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

#include "ngsi/Restriction.h"
#include "ngsi/Reference.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextAttribute.h"
#include "cache/SubscriptionCache.h"

namespace orion
{

/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription()
{
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription(std::string _subscriptionId)
{
  subscriptionId = _subscriptionId;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription
(
  std::string                _subscriptionId,
  std::vector<EntityInfo*>*  _entityIdInfos,
  std::vector<std::string>&  _attributes,
  int64_t                    _throttling.
  int64_t                    _expirationTime,
  Restriction&               _restriction,
  NotifyConditionVector&     _notifyConditionVector,
  Reference&                 _reference
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

  reference.set(_reference);
}

}
