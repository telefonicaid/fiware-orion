/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                          // LM_*

#include "cache/CachedSubscription.h"                               // CachedSubscription

#include "orionld/common/orionldState.h"                            // promNotifications
#include "orionld/prometheus/promCounterIncrease.h"                 // promCounterIncrease
#include "orionld/mongoc/mongocSubCountersUpdate.h"                 // mongocSubCountersUpdate
#include "orionld/notifications/notificationSuccess.h"              // Own interface



// -----------------------------------------------------------------------------
//
// notificationSuccess -
//
void notificationSuccess(CachedSubscription* subP, const double timestamp)
{
  subP->lastSuccess           = timestamp;
  subP->lastNotificationTime  = timestamp;
  subP->consecutiveErrors     = 0;
  subP->count                += 1;
  subP->dirty                += 1;

  promCounterIncrease(promNotifications);

  // LM(("SUBC: dirty: %d, cSubCounters: %d", subP->dirty, cSubCounters));

  //
  // Flush to DB?
  // - If subP->dirty (number of counter updates since last flush) >= cSubCounters
  //   - AND cSubCounters != 0
  //
  if ((cSubCounters != 0) && (subP->dirty >= cSubCounters))
  {
    // LM(("SUBC: Calling mongocSubCountersUpdate"));
    mongocSubCountersUpdate(subP, subP->count, subP->lastNotificationTime, subP->lastFailure, subP->lastSuccess, false, true);
    subP->dirty    = 0;
    subP->dbCount += subP->count;
    subP->count    = 0;
  }
}
