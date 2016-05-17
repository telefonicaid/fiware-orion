#ifndef SRC_LIB_MONGO_BACKEND_MONGOCOMMONSUBSCRIPTION_H
#define SRC_LIB_MONGO_BACKEND_MONGOCOMMONSUBSCRIPTION_H

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
*
*/
extern std::string setNewSubscriptionId(mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setExpiration -
*
*/
extern void setExpiration(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setHttpInfo -
*
*/
extern void setHttpInfo(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setThrottling -
*
*/
extern void setThrottling(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setServicePath -
*
*/
extern void setServicePath(std::string servicePath, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setDescription -
*
*/
extern void setDescription(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setStatus -
*
*/
extern void setStatus(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setEntities -
*
*/
extern void setEntities(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);


/* ****************************************************************************
*
* setAttrs -
*
*/
extern void setAttrs(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setCondsAndInitialNotify -
*
*/
extern void setCondsAndInitialNotify
(
  const ngsiv2::Subscription&      sub,
  const std::string&               subId,
  const std::string&               status,
  const std::string&               url,
  RenderFormat                     attrsFormat,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  BSONObjBuilder*                  b,
  bool*                            notificationDone
);



/* ****************************************************************************
*
* setLastNotification -
*
*/
extern void setLastNotification(long long lastNotification, BSONObjBuilder* b);



/* ****************************************************************************
*
* setCount -
*
*/
extern void setCount(long long count, BSONObjBuilder* b);



/* ****************************************************************************
*
* setExpression -
*
*/
extern void setExpression(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);



/* ****************************************************************************
*
* setFormat -
*
*/
extern void setFormat(const ngsiv2::Subscription& sub, mongo::BSONObjBuilder* b);


#endif // SRC_LIB_MONGO_BACKEND_MONGOCOMMONSUBSCRIPTION_H
