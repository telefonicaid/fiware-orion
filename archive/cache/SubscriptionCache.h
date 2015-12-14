#ifndef SRC_LIB_CACHE_SUBSCRIPTIONCACHE_H_
#define SRC_LIB_CACHE_SUBSCRIPTIONCACHE_H_

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
#include "cache/Subscription.h"

struct SubscribeContextRequest;
struct UpdateContextSubscriptionRequest;

using namespace mongo;

namespace orion
{


/* ****************************************************************************
*
* SubscriptionCache -
*
* The class fields:
* -------------------------------------------------------------------------------
*   subs:            the vector of Subscriptions
*   mutex:           binary semaphore protecting 'subs'
*
*   init():          at startup, fill cache from database
*   insert():        insert/modify a Subscription in 'subs'
*   delete():        delete a Subscription from 'subs'
*   refresh():       synchronize cache with DB
*   lookup():        find a subscription based on entity and attribute
*   lookupById():    find a subscription based on subscriptionId
*
*/
class SubscriptionCache
{
private:
  std::vector<Subscription*>  subs;
  std::string                 dbPrefix;
  sem_t                       mutex;
  pthread_t                   mutexOwner;

public:
  SubscriptionCache();
  SubscriptionCache(std::string _dbPrefix);

  void           init(void);
  void           fillFromDb(void);
  void           insert(Subscription* sub);
  void           insert(const std::string& tenant, BSONObj bobj);
  int            remove(Subscription* sub);
  int            remove(const std::string&  tenant,
                        const std::string&  servicePath,
                        const std::string&  subId);
  void           release();
  int            refresh(void);
  void           present(const std::string& prefix);
  int            size(void) { return subs.size(); }
  Subscription*  lookupById(const std::string&       tenant,
                            const std::string&       servicePath,
                            const std::string&       subId);

  Subscription*  lookupById(const std::string&       subId);

  void           lookup(const std::string&           tenant,
                        const std::string&           servicePath,
                        const std::string&           id,
                        const std::string&           type,
                        const std::string&           attributeName,
                        std::vector<Subscription*>*  subV);

  void           semTake(void);
  void           semGive(void);

private:
  void           semInit(void);
};

}  // namespace orion

#endif  // SRC_LIB_CACHE_SUBSCRIPTIONCACHE_H_
