#ifndef SRC_LIB_NGSI10_NOTIFYCONTEXTREQUEST_H_
#define SRC_LIB_NGSI10_NOTIFYCONTEXTREQUEST_H_

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
* Author: Fermin Galan
*/
#include <string>

#include "common/RenderFormat.h"
#include "ngsi/Request.h"
#include "ngsi/SubscriptionId.h"
#include "ngsi/Originator.h"
#include "ngsi/ContextElementResponseVector.h"



/* ****************************************************************************
*
* NotifyContextRequest -
*/
typedef struct NotifyContextRequest
{
  SubscriptionId                subscriptionId;                // Mandatory
  Originator                    originator;                    // Mandatory
  ContextElementResponseVector  contextElementResponseVector;  // Optional

  std::string   render( bool asJsonObject);
  std::string   toJson(RenderFormat                     renderFormat,
                       const std::vector<std::string>&  metadataFilter);
  std::string   check(ApiVersion apiVersion, const std::string& predetectedError);
  void          release(void);
  NotifyContextRequest* clone(void);
} NotifyContextRequest;

#endif  // SRC_LIB_NGSI10_NOTIFYCONTEXTREQUEST_H_
