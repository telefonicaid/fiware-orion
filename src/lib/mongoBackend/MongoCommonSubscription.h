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

#include "mongo/client/dbclient.h"
#include "apiTypesV2/Subscription.h"



/* ****************************************************************************
*
* setNewSubscriptionId -
*/
extern std::string setNewSubscriptionId(mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setExpiration -
*/
extern void setExpiration(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setHttpInfo -
*/
extern void setHttpInfo(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setThrottling -
*/
extern void setThrottling(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setServicePath -
*/
extern void setServicePath(const std::string& servicePath, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setDescription -
*/
extern void setDescription(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setStatus -
*/
extern void setStatus(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setEntities -
*/
extern void setEntities(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setAttrs -
*/
extern void setAttrs(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setCondsAndInitialNotify -
*/
extern void setCondsAndInitialNotify
(
  const ngsiv2::Subscription&      sub,
  const std::string&               subId,
  const std::string&               status,
  const std::vector<std::string>&  notifAttributesV,
  const std::vector<std::string>&  metadataV,
  const ngsiv2::HttpInfo&          httpInfo,
  bool                             blacklist,
  RenderFormat                     attrsFormat,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  mongo::BSONObjBuilder*           b,
  bool*                            notificationDone
);



/* ****************************************************************************
*
* setLastNotification -
*/
extern void setLastNotification(long long lastNotification, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setCount -
*/
extern void setCount(long long count, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setLastFailure -
*/
extern void setLastFailure(long long lastFailure, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setLastSuccess -
*/
extern void setLastSuccess(long long lastSuccess, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setExpression -
*/
extern void setExpression(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setFormat -
*/
extern void setFormat(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setBlacklist -
*/
extern void setBlacklist(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setMetadata -
*/
extern void setMetadata(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



#ifdef ORIONLD
/* ****************************************************************************
*
* setContext -
*/
extern void setContext(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* bobP);



/* ****************************************************************************
*
* setSubscriptionId -
*/
extern std::string setSubscriptionId(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* bobP);



/* ****************************************************************************
*
* setName -
*/
extern void setName(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* bobP);



/* ****************************************************************************
*
* setMimeType -
*/
extern void setMimeType(ngsiv2::Subscription* subP, mongo::BSONObjBuilder* bobP);



/* ****************************************************************************
*
* setCsf -
*/
extern void setCsf(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* bobP);



/* ****************************************************************************
*
* setTimeInterval - 
*/
extern void setTimeInterval(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* bobP);

#endif

#endif  // SRC_LIB_MONGOBACKEND_MONGOCOMMONSUBSCRIPTION_H_
