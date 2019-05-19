#ifndef SRC_LIB_MONGOBACKEND_MONGOGETSUBSCRIPTIONS_H_
#define SRC_LIB_MONGOBACKEND_MONGOGETSUBSCRIPTIONS_H_

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
* Author: Orion dev team
*/
#include <string>
#include <vector>
#include <map>

#include "rest/OrionError.h"
#include "apiTypesV2/Subscription.h"



/* ****************************************************************************
*
* mongoListSubscriptions -
*/
extern void mongoListSubscriptions
(
  std::vector<ngsiv2::Subscription>*   vec,
  OrionError*                          oe,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   tenant,
  const std::string&                   servicePath,
  int                                  limit,
  int                                  offset,
  long long*                           count
);



/* ****************************************************************************
*
* mongoGetSubscription -
*/
extern void mongoGetSubscription
(
  ngsiv2::Subscription*                sub,
  OrionError*                          oe,
  const std::string&                   idSub,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   tenant
);



#ifdef ORIONLD
/* ****************************************************************************
*
* mongoGetLdSubscription -
*/
extern bool mongoGetLdSubscription
(
  ngsiv2::Subscription*  subP,
  const char*            subId,
  const char*            tenant,
  HttpStatusCode*        statusCodeP,
  char**                 detailsP
);



/* ****************************************************************************
*
* mongoGetLdSubscriptions - 
*/
extern bool mongoGetLdSubscriptions
(
  ConnectionInfo*                     ciP,
  std::vector<ngsiv2::Subscription>*  subVecP,
  const char*                         tenant,
  long long*                          countP,
  OrionError*                         oeP
);

#endif  // ORIONLD

#endif  // SRC_LIB_MONGOBACKEND_MONGOGETSUBSCRIPTIONS_H_
