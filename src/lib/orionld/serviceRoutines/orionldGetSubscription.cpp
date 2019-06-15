/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderAdd
#include "mongoBackend/mongoGetSubscriptions.h"                  // mongoGetLdSubscription
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/OrionldConnection.h"                    // orionldState
#include "orionld/context/orionldCoreContext.h"                  // orionldDefaultContext
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

  LM_T(LmtServiceRoutine, ("In orionldGetSubscription (%s)", orionldState.wildcard[0]));
  LM_TMP(("TENANT: %s", orionldState.tenant));

  if (mongoGetLdSubscription(&subscription, orionldState.wildcard[0], orionldState.tenant, &ciP->httpStatusCode, &details) != true)
  {
    LM_E(("mongoGetLdSubscription error: %s", details));
    orionldErrorResponseCreate(ciP, OrionldResourceNotFound, details, orionldState.wildcard[0], OrionldDetailsString);
    return false;
  }

  // Transform to KjNode tree
  ciP->httpStatusCode       = SccOk;
  orionldState.responseTree = kjTreeFromSubscription(ciP, &subscription);

  return true;
}
