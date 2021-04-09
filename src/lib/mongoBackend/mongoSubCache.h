#ifndef SRC_LIB_MONGOBACKEND_MONGOSUBCACHE_H_
#define SRC_LIB_MONGOBACKEND_MONGOSUBCACHE_H_

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
#include "rest/StringFilter.h"

#include "mongoDriver/BSONObj.h"



/* ****************************************************************************
*
* mongoSubCacheItemInsert - 
*/
extern int mongoSubCacheItemInsert(const char* tenant, const orion::BSONObj& sub);



/* ****************************************************************************
*
* mongoSubCacheItemInsert - 
*/
extern int mongoSubCacheItemInsert
(
  const char*            tenant,
  const orion::BSONObj&  sub,
  const char*            subscriptionId,
  const char*            servicePath,
  int                    lastNotificationTime,
  long long              expirationTime,
  const std::string&     status,
  const std::string&     q,
  const std::string&     mq,
  const std::string&     geometry,
  const std::string&     coords,
  const std::string&     georel,
  StringFilter*          stringFilterP,
  StringFilter*          mdStringFilterP,
  RenderFormat           renderFormat
);



/* ****************************************************************************
*
* mongoSubCacheRefresh - 
*/
extern void mongoSubCacheRefresh(const std::string& database);



/* ****************************************************************************
*
* mongoSubCountersUpdate - 
*/
extern void mongoSubCountersUpdate
(
  const std::string&  tenant,
  const std::string&  subId,
  long long           count,
  long long           lastNotificationTime,
  long long           lastFailure,
  long long           lastSuccess,
  const std::string&  failureReason,
  long long           statusCode
);

#endif  // SRC_LIB_MONGOBACKEND_MONGOSUBCACHE_H_
