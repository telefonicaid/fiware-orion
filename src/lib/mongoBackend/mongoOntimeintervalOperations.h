#ifndef MONGO_ONTIMEINTERVAL_OPERATIONS_H
#define MONGO_ONTIMEINTERVAL_OPERATIONS_H

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan Marquez
*/
#include <string>

#include "ngsiNotify/ContextSubscriptionInfo.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/AttributeList.h"
#include "ngsi10/NotifyContextRequest.h"

/* ****************************************************************************
*
* mongoGetContextSubscriptionInfo -
*/
extern HttpStatusCode mongoGetContextSubscriptionInfo(const std::string& subId, ContextSubscriptionInfo* csiP, std::string* err, const std::string& tenant = "");

/* ****************************************************************************
*
* mongoGetContextElementResponses -
*/
extern HttpStatusCode mongoGetContextElementResponses(const EntityIdVector& enV, const AttributeList& attrL, ContextElementResponseVector* cerV, std::string* err, const std::string& tenant = "");

/* ****************************************************************************
*
* mongoUpdateCsubNewNotification -
*/
extern HttpStatusCode mongoUpdateCsubNewNotification(const std::string& subId, std::string* err, const std::string& tenant = "");

#endif
