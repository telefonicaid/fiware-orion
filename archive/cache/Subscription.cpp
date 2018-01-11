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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "cache/Subscription.h"
#include "mongoBackend/MongoGlobal.h"


namespace orion
{

/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription()
{
  tenant                = "";
  servicePath           = "";
  subscriptionId        = "";
  throttling            = 0;
  expirationTime        = 0;
  lastNotificationTime  = -1;
  pendingNotifications  = 0;
  format                = XML;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription
(
  const std::string& _tenant,
  const std::string& _servicePath,
  const std::string& _subscriptionId,
  Format             _format
)
{
  tenant                = _tenant;
  servicePath           = _servicePath;
  subscriptionId        = _subscriptionId;
  throttling            = 0;
  expirationTime        = 0;
  lastNotificationTime  = -1;
  pendingNotifications  = 0;
  format                = _format;
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription
(
  const std::string&        _tenant,
  const std::string&        _servicePath,
  SubscribeContextRequest*  scrP,
  std::string               _subscriptionId,
  int64_t                   _expiration,
  Format                    _format
)
{
  unsigned int ix;

  //
  // 0. Fill in 'constant initial values'
  //
  lastNotificationTime  = -1;
  pendingNotifications  = 0;
  format                = XML;



  //
  // 1. Get Tenant, ServicePath and Subscription Id
  //
  tenant         = _tenant;
  servicePath    = _servicePath;
  subscriptionId = _subscriptionId;
  format         = _format;

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
  for (ix = 0; ix < scrP->attributeList.stringV.size(); ++ix)
  {
    attributes.push_back(scrP->attributeList.stringV[ix]);
  }


  //
  // 4. throttling
  //
  throttling = scrP->throttling.parse();


  //
  // 5. expirationTime
  //
  expirationTime = _expiration;


  //
  // 6. restriction
  //
  restriction.fill(&scrP->restriction);


  //
  // 7. notifyConditionVector
  //
  notifyConditionVector.fill(scrP->notifyConditionVector);


  //
  // 8. reference
  //
  reference.set(scrP->reference.get());
}



/* ****************************************************************************
*
* Subscription::Subscription - 
*/
Subscription::Subscription
(
  const std::string&               _tenant,
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
  Format                           _format
)
{
  unsigned int ix;

  tenant         = _tenant;
  servicePath    = _servicePath;
  subscriptionId = _subscriptionId;
  format         = _format;

  for (ix = 0; ix < _entityIdInfos.size(); ++ix)
  {
    entityIdInfos.push_back(_entityIdInfos[ix]);
  }

  for (ix = 0; ix < _attributes.size(); ++ix)
  {
    attributes.push_back(_attributes[ix]);
  }

  throttling            = _throttling;
  expirationTime        = _expirationTime;
  lastNotificationTime  = _lastNotificationTime;

  restriction.fill((Restriction*) &_restriction);
  notifyConditionVector.fill(_notifyConditionVector);
  reference.set(_reference);
}



/* ****************************************************************************
*
* Subscription::entityIdInfoAdd - 
*/
void Subscription::entityIdInfoAdd(EntityInfo* entityIdInfoP)
{
  entityIdInfos.push_back(entityIdInfoP);
}



/* ****************************************************************************
*
* Subscription::attributeMatch - 
*/
bool Subscription::attributeMatch(const std::string& attributeName)
{
  for (unsigned int ncvIx = 0; ncvIx < notifyConditionVector.size(); ++ncvIx)
  {
    NotifyCondition* ncP = notifyConditionVector[ncvIx];

    for (unsigned int cvIx = 0; cvIx < ncP->condValueList.size(); ++cvIx)
    {
      if (ncP->condValueList[cvIx] == attributeName)
      {
        return true;
      }
    }
  }

  return false;
}



/* ****************************************************************************
*
* Subscription::match - 
*/
bool Subscription::match
(
  const std::string&  _tenant,
  const std::string&  _servicePath,
  const std::string&  id,
  const std::string&  type,
  const std::string&  attributeName
)
{
  if (_tenant != tenant)
  {
    return false;
  }

  if (servicePathMatch(_servicePath) == false)
  {
    return false;
  }


  //
  // If ONCHANGE and one of the attribute names in the scope vector
  // of the subscription has the same name as the incoming attribute. there is a match.
  //
  if (!attributeMatch(attributeName))
  {
    return false;
  }

  for (unsigned int ix = 0; ix < entityIdInfos.size(); ++ix)
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
* Subscription::servicePathMatch - 
*/
bool Subscription::servicePathMatch
(
  const std::string&  _servicePathIn
)
{
  const char*  spath;
  std::string  _servicePath = _servicePathIn;

  //
  // Empty service path?
  //
  if ((_servicePath.length() == 0) && (servicePath.length() == 0))
  {
    return true;
  }

  //
  // Default service path for a subscription is "/"
  //
  if (_servicePath.length() == 0)
  {
    _servicePath = "/";
  }

  spath = servicePath.c_str();

  //
  // No wildcard - exact match
  //
  if (spath[strlen(spath) - 1] != '#')
  {
    return (_servicePath == servicePath);
  }

  // Special case. "/#" matches EVERYTHING
  if (servicePath == "/#")
  {
    return true;
  }


  //
  // Wildcard - remove '#' and compare to the length of _servicePath.
  //            If equal upto the length of _servicePath, then it is a match
  //
  // Actually, there is more than one way to match here:
  // If we have a servicePath of the subscription as 
  // "/a/b/#", then the following service-paths must match:
  //   1. /a/b
  //   2. /a/b/ AND /a/b/.+  (any path below /a/b/)
  //
  // What should NOT match is "/a/b2".
  //
  unsigned int len = strlen(spath) - 2;

  // 1. /a/b - (removing 2 chars from the cache-subscription removes "/#"

  if ((spath[len] == '/') && (strlen(_servicePath.c_str()) == len) && (strncmp(spath, _servicePath.c_str(), len) == 0))
  {
    return true;
  }

  // 2. /a/b/.+
  len = strlen(spath) - 1;
  if (strncmp(spath, _servicePath.c_str(), len) == 0)
  {
    return true;
  }
  
  return false;
}



/* ****************************************************************************
*
* Subscription::hasAttribute - 
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
* Subscription::release - 
*/
void Subscription::release(void)
{
  restriction.release();
  notifyConditionVector.release();

  for (unsigned int ix = 0; ix < entityIdInfos.size(); ++ix)
  {
    entityIdInfos[ix]->release();
    delete(entityIdInfos[ix]);
  }

  entityIdInfos.clear();
  attributes.clear();
}



/* ****************************************************************************
*
* Subscription::present - 
*/
void Subscription::present(const std::string& prefix)
{
  LM_T(LmtPresent, ("%stenant:               %s", prefix.c_str(), tenant.c_str()));
  LM_T(LmtPresent, ("%sservicePath:          %s", prefix.c_str(), servicePath.c_str()));
  LM_T(LmtPresent, ("%ssubscriptionId:       %s", prefix.c_str(), subscriptionId.c_str()));

  LM_T(LmtPresent, ("%s%d items of EntityInfo:", prefix.c_str(), entityIdInfos.size()));
  for (unsigned int ix = 0; ix < entityIdInfos.size(); ++ix)
  {
    entityIdInfos[ix]->present(prefix + "  ");
  }

  LM_T(LmtPresent, ("%s%d attributes", prefix.c_str(), attributes.size()));
  for (unsigned int ix = 0; ix < attributes.size(); ++ix)
  {
    LM_T(LmtPresent, ("%s  %s", prefix.c_str(), attributes[ix].c_str()));
  }


  LM_T(LmtPresent, ("%sthrottling:           %lu", prefix.c_str(), throttling));
  LM_T(LmtPresent, ("%sexpirationTime:       %lu", prefix.c_str(), expirationTime));
  
  notifyConditionVector.present(prefix + "  ");
  restriction.present(prefix + "  ");
  LM_T(LmtPresent, ("%sreference:            %s",  prefix.c_str(), reference.get().c_str()));

  LM_T(LmtPresent, ("%slastNotificationTime: %lu",  prefix.c_str(), lastNotificationTime));
  LM_T(LmtPresent, ("%spendingNotifications: %lu",  prefix.c_str(), pendingNotifications));
  LM_T(LmtPresent, ("%snotification format:  %s",   prefix.c_str(), (format == XML)? "XML" : "JSON"));
}

}
