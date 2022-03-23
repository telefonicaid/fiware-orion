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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                   // LM_*

#include "cache/subCache.h"                                  // CachedSubscription, subCacheMatch

#include "orionld/common/orionldState.h"                     // orionldState
#include "orionld/types/OrionldAlteration.h"                 // OrionldAlteration, orionldAlterationType
#include "orionld/notifications/subCacheAlterationMatch.h"   // subCacheAlterationMatch
#include "orionld/notifications/orionldAlterationsTreat.h"   // Own interface



// -----------------------------------------------------------------------------
//
// orionldAlterationsTreat -
//
// As a first "draft", just find subs in the sub-cache and send notifications, one by one
// Later - build lists of notifications and make sure one single notification in sent to each subscriber
// Also, instead of searching in the sub-cache, search in the DB, if the sub-cache is OFF
//
//
// IMPLEMENTATION DETAILS
//   I will need a new subCacheMatch function (subCacheAlterationMatch) - that accepts altList as input and gives back a list of
//   [ { CachedSubscription*, OrionldAttributeAlteration* }, {} ]
//
//   Cause, OrionldAttributeAlteration has the OrionldAlterationType, and we need it
//
//   Also, we need the complete entity as queried from DB and merged with the new, so we can include it in the notification
//   Remember - a PUT doesn't need to query the DB before replacing, but is already contains the entire entity - we need also
//   createdAt and modifiedAt, in case the subscription asks for sysAttrs
//
//   I added a field "KjNode* entityP" in struct OrionldAlteration.
//   This KjNode pointer needs to reference the entity as it is AFTER the alteration.
//
void orionldAlterationsTreat(OrionldAlteration* altList)
{
  LM_TMP(("KZ: Alterations present"));

  OrionldAlterationMatch* matchList;
  int                     matches;

  matchList = subCacheAlterationMatch(altList, &matches);

  if (matchList == NULL)
    LM_RVE(("KZ: No matching subscriptions"));

  LM_TMP(("KZ: %d Matching subscriptions for alteration", matches));
  LM_TMP(("KZ: %llu Matching subs", matches));

  for (OrionldAlterationMatch* matchP = matchList; matchP != NULL; matchP = matchP->next)
  {
    ngsiv2::HttpInfo* hiP = &matchP->subP->httpInfo;

    LM_TMP(("KZ: Got a %s Notification for sub %s: %s %s", orionldAlterationType(matchP->altAttrP->alterationType), matchP->subP->subscriptionId, verbName(hiP->verb), hiP->url.c_str()));
  }
}
