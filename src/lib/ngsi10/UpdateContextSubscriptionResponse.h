#ifndef SRC_LIB_NGSI10_UPDATECONTEXTSUBSCRIPTIONRESPONSE_H_
#define SRC_LIB_NGSI10_UPDATECONTEXTSUBSCRIPTIONRESPONSE_H_

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
* Author: Ken Zangelin
*/
#include <string>

#include "ngsi/SubscribeResponse.h"
#include "ngsi/SubscribeError.h"
#include "ngsi/StatusCode.h"



/* ****************************************************************************
*
* UpdateContextSubscriptionResponse - 
*/
typedef struct UpdateContextSubscriptionResponse
{
  SubscribeResponse     subscribeResponse;     // Optional - but one of the two MUST be present!
  SubscribeError        subscribeError;        // Optional - but one of the two MUST be present!

  UpdateContextSubscriptionResponse();
  UpdateContextSubscriptionResponse(StatusCode& errorCode);
  ~UpdateContextSubscriptionResponse();

  std::string toJsonV1(void);
} UpdateContextSubscriptionResponse;

#endif  // SRC_LIB_NGSI10_UPDATECONTEXTSUBSCRIPTIONRESPONSE_H_
