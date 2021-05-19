/*
*
* Copyright 2018 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderAdd
#include "mongoBackend/mongoGetSubscriptions.h"                  // mongoGetLdSubscription
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjTreeFromSubscription.h"               // kjTreeFromSubscription
#include "orionld/serviceRoutines/orionldGetSubscription.h"      // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetSubscription -
//
bool orionldGetSubscription(ConnectionInfo* ciP)
{
  ngsiv2::Subscription  subscription;
  char*                 details;

  subscription.descriptionProvided = false;
  subscription.expires             = -1;  // 0?
  subscription.throttling          = -1;  // 0?
  subscription.timeInterval        = -1;  // 0?

  if (mongoGetLdSubscription(&subscription, orionldState.wildcard[0], orionldState.tenant, &orionldState.httpStatusCode, &details) != true)
  {
    LM_E(("mongoGetLdSubscription error: %s", details));
    orionldErrorResponseCreate(OrionldResourceNotFound, details, orionldState.wildcard[0]);
    return false;
  }

  // Transform to KjNode tree
  orionldState.httpStatusCode = SccOk;
  orionldState.responseTree   = kjTreeFromSubscription(&subscription);

  return true;
}
