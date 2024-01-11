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
* Author: Ken Zangelin and Gabriel Quaresma
*/
#include "logMsg/logMsg.h"                                       // LM_*

#include "cache/subCache.h"                                      // CachedSubscription, subCacheItemLookup, ...

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_URI
#include "orionld/mqtt/mqttDisconnect.h"                         // mqttDisconnect
#include "orionld/mongoc/mongocSubscriptionLookup.h"             // mongocSubscriptionLookup
#include "orionld/mongoc/mongocSubscriptionDelete.h"             // mongocSubscriptionDelete
#include "orionld/legacyDriver/legacyDeleteSubscription.h"       // legacyDeleteSubscription
#include "orionld/serviceRoutines/orionldDeleteSubscription.h"   // Own Interface



// ----------------------------------------------------------------------------
//
// orionldDeleteSubscription -
//
bool orionldDeleteSubscription(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))
    return legacyDeleteSubscription();

  PCHECK_URI(orionldState.wildcard[0], true, 0, "Invalid Subscription Identifier", orionldState.wildcard[0], 400);

  KjNode* subP = mongocSubscriptionLookup(orionldState.wildcard[0]);
  if (subP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Subscription not found", orionldState.wildcard[0], 404);
    return false;
  }

  if (mongocSubscriptionDelete(orionldState.wildcard[0]) == false)
    return false;  // mongocSubscriptionDelete calls orionldError, setting status code to 500

  CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, orionldState.wildcard[0]);

  if (cSubP == NULL)
  {
    if (noCache == false)
      LM_W(("The subscription '%s' was successfully removed from DB but does not exist in sub-cache ... (sub-cache is enabled)"));

    //
    // FIXME: If mqtt, we need to disconnect from MQTT broker
    //        BUT, not until Orion-LD is able to run without sub-cache
    //
  }
  else
  {
    // If MQTT subscription - disconnect from mqtt broker
    if (cSubP->protocol == MQTT)
    {
      MqttInfo* mqttP = &cSubP->httpInfo.mqtt;
      mqttDisconnect(mqttP->host, mqttP->port, mqttP->username, mqttP->password, mqttP->version);
    }
    subCacheItemRemove(cSubP);
  }

  orionldState.httpStatusCode = 204;

  return true;
}
