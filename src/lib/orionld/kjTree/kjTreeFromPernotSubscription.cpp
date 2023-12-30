/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/PernotSubscription.h"                    // PernotSubscription
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/dbModel/dbModelValueStrip.h"                   // dbModelValueStrip
#include "orionld/q/qAliasCompact.h"                             // qAliasCompact



// -----------------------------------------------------------------------------
//
// NULL_CHECK -
//
#define NULL_CHECK(pointer) if (pointer == NULL) return outOfMemory()



// -----------------------------------------------------------------------------
//
// outOfMemory -
//
static KjNode* outOfMemory(void)
{
  LM_E(("Internal Error (out of memory creating a KjNode-tree for a PernotSubscription)"));
  return NULL;
}



// -----------------------------------------------------------------------------
//
// timestampAdd -
//
static void timestampAdd(KjNode* containerP, const char* fieldName, double ts)
{
  if (ts <= 0)
    return;

  char buf[64];
  if (numberToDate(ts, buf, sizeof(buf) - 1) == true)
  {
    KjNode* nodeP = kjString(orionldState.kjsonP, fieldName, buf);
    kjChildAdd(containerP, nodeP);
  }
}



// -----------------------------------------------------------------------------
//
// counterAdd -
//
static void counterAdd(KjNode* containerP, const char* fieldName, uint32_t count)
{
  if (count == 0)
    return;

  KjNode* nodeP = kjInteger(orionldState.kjsonP, fieldName, count);
  kjChildAdd(containerP, nodeP);
}



// -----------------------------------------------------------------------------
//
// kjTreeFromPernotSubscription - in NGSI-LD API format
//
KjNode* kjTreeFromPernotSubscription(PernotSubscription* pSubP, bool sysAttrs, bool contextInBody)
{
  KjNode* nodeP;

  //
  // Top level object - the Subscription
  //
  // 1. We start with what we already get from the Pernot sub-cache.
  //
  // 2. And, we remove the sysAttrs, unless asked for,
  //
  // 3. We add the "volatile" fields:
  //   "isActive",
  //   "status"
  //   "notification::status"
  //
  // 4. We add counters and timestamps:
  // - timesSent
  // - timesFailed
  // - lastNotification
  // - lastFailure
  // - lastSuccess
  //
  // 5. And we compact all fields that need it, according with the current @context
  //
  // 6. Compact attribute names in q
  //
  KjNode* sP = kjClone(orionldState.kjsonP, pSubP->kjSubP);
  NULL_CHECK(sP);

  KjNode* notificationP = kjLookup(sP, "notification");
  if (notificationP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (subscription without notification field)", pSubP->subscriptionId, 500);
    return NULL;
  }

  // 2. Remove modifiedAt, createdAt?
  if (sysAttrs == false)
  {
    nodeP = kjLookup(sP, "modifiedAt");
    if (nodeP != NULL)
      kjChildRemove(sP, nodeP);

    nodeP = kjLookup(sP, "createdAt");
    if (nodeP != NULL)
      kjChildRemove(sP, nodeP);
  }

  // 3. Add the "volatile" fields, but, first decide the states
  bool  isActive           = true;
  char* status             = (char*) "active";
  char* notificationStatus = (char*) "ok";
  //
  // Lookup and compare with:
  // * expiresAt
  // * any error
  // * paused, ...
  //
  KjNode* isActiveP = kjBoolean(orionldState.kjsonP, "isActive", isActive);
  kjChildAdd(sP, isActiveP);
  KjNode* statusP = kjString(orionldState.kjsonP, "status", status);
  kjChildAdd(sP, statusP);
  KjNode* notificationStatusP = kjString(orionldState.kjsonP, "status", notificationStatus);
  kjChildAdd(notificationP, notificationStatusP);

  // 4. counters and timestamps
  counterAdd(notificationP,   "timesSent",        pSubP->notificationAttemptsDb + pSubP->notificationAttempts);
  counterAdd(notificationP,   "timesFailed",      pSubP->notificationErrorsDb + pSubP->notificationErrors);
  timestampAdd(notificationP, "lastNotification", pSubP->lastNotificationTime);
  timestampAdd(notificationP, "lastSuccess",      pSubP->lastSuccessTime);
  timestampAdd(notificationP, "lastFailure",      pSubP->lastFailureTime);

  // 5.1. Compact entity types
  KjNode* entitiesP = kjLookup(sP, "entities");
  if (entitiesP != NULL)
  {
    for (KjNode* entitiesItemP = entitiesP->value.firstChildP; entitiesItemP != NULL; entitiesItemP = entitiesItemP->next)
    {
      KjNode* typeP = kjLookup(entitiesItemP, "type");

      if (typeP != NULL)
        typeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
    }
  }

  // 5.2. Compact notification::attributes
  KjNode* attributesP = kjLookup(notificationP, "attributes");
  if (attributesP != NULL)
  {
    for (KjNode* attributeP = attributesP->value.firstChildP; attributeP != NULL; attributeP = attributeP->next)
    {
      attributeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, attributeP->value.s, NULL, NULL);
    }
  }

  //
  // 6. Compact attribute names in q
  //
  KjNode* qP = kjLookup(sP, "q");
  if (qP != NULL)
  {
    LM_T(LmtPernot, ("Found 'q' (%s)- fixing it", qP->value.s));
    dbModelValueStrip(qP);
    LM_T(LmtPernot, ("'q' after dbModelValueStrip: '%s'", qP->value.s));
    qAliasCompact(qP, true);  // qAliasCompact uses orionldState.contextP - which is what we want
    qP->name = (char*) "q";
    LM_T(LmtPernot, ("'q' final: '%s'", qP->value.s));
  }
  else
    LM_T(LmtPernot, ("No 'q'"));

  //
  // name => subscriptionName
  // FIXME: don't do this every time!
  //        do it in pernotSubCacheAdd!
  //
  KjNode* nameP = kjLookup(sP, "name");
  if (nameP != NULL)
    nameP->name = (char*) "subscriptionName";

  return sP;
}
