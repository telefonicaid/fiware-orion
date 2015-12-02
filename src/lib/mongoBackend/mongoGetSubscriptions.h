#ifndef MONGOLISTSUBSCRIPTIONS_H
#define MONGOLISTSUBSCRIPTIONS_H

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

#include <map>
#include <string>
#include <vector>

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

#endif  // MONGOLISTSUBSCRIPTIONS_H
