#ifndef SRC_LIB_MONGOBACKEND_MONGOUNSUBSCRIBECONTEXT_H_
#define SRC_LIB_MONGOBACKEND_MONGOUNSUBSCRIBECONTEXT_H_

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

#include "rest/HttpStatusCode.h"
#include "ngsi10/UnsubscribeContextRequest.h"
#include "ngsi10/UnsubscribeContextResponse.h"



/* ****************************************************************************
*
* mongoUnsubscribeContext - 
*/
extern HttpStatusCode mongoUnsubscribeContext
(
  UnsubscribeContextRequest*   requestP,
  UnsubscribeContextResponse*  responseP,
  const std::string&           tenant = ""
);



#ifdef ORIONLD
/* ****************************************************************************
*
* mongoDeleteLdSubscription -
*/
extern bool mongoDeleteLdSubscription
(
  const char*  subId,
  const char*  tenant,
  int*         statusCodeP,
  char**       details
);
#endif  // ORIONLD

#endif  // SRC_LIB_MONGOBACKEND_MONGOUNSUBSCRIBECONTEXT_H_
