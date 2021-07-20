#ifndef SRC_LIB_MONGOBACKEND_MONGOCOMMONSUBSCRIPTION_H_
#define SRC_LIB_MONGOBACKEND_MONGOCOMMONSUBSCRIPTION_H_

/*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*
*/
#include <string>
#include <vector>

#include "apiTypesV2/Subscription.h"

#include "mongoDriver/BSONObjBuilder.h"



/* ****************************************************************************
*
* setNewSubscriptionId -
*/
extern std::string setNewSubscriptionId(orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setExpiration -
*/
extern void setExpiration(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setHttpInfo -
*/
extern void setHttpInfo(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setThrottling -
*/
extern void setThrottling(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setMaxFailsLimit -
*/
extern void setMaxFailsLimit(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);


/* ****************************************************************************
*
* setServicePath -
*/
extern void setServicePath(const std::string& servicePath, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setDescription -
*/
extern void setDescription(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setStatus -
*/
extern void setStatus(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setEntities -
*/
extern void setEntities(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setAttrs -
*/
extern void setAttrs(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setConds
*/
extern void setConds
(
  const ngsiv2::Subscription&      sub,
  const std::vector<std::string>&  notifAttributesV,
  orion::BSONObjBuilder*           b
);



/* ****************************************************************************
*
* setLastNotification -
*/
extern void setLastNotification(long long lastNotification, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setCount -
*/
extern void setCount(long long count, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setLastFailure -
*/
extern void setLastFailure(long long lastFailure, const std::string& lastFailureReason, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setLastSuccess -
*/
extern void setLastSuccess(long long lastSuccess, long long lastSuccessCode, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setExpression -
*/
extern void setExpression(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setFormat -
*/
extern void setFormat(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setBlacklist -
*/
extern void setBlacklist(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setOnlyChanged -
*/
extern void setOnlyChanged(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);



/* ****************************************************************************
*
* setMetadata -
*/
extern void setMetadata(const ngsiv2::Subscription& sub, orion::BSONObjBuilder* b);

#endif  // SRC_LIB_MONGOBACKEND_MONGOCOMMONSUBSCRIPTION_H_
