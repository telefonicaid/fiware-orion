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
#include <pthread.h>                                        // pthread_exit

extern "C"
{
#include "kalloc/kaBufferInit.h"                            // kaBufferInit
#include "kalloc/kaBufferReset.h"                           // kaBufferReset
#include "kjson/kjBufferCreate.h"                           // kjBufferCreate
#include "kjson/KjNode.h"                                   // KjNode
#include "kjson/kjBuilder.h"                                // kjArray, ...
}

#include "logMsg/logMsg.h"                                  // LM_x

#include "orionld/types/OrionldRenderFormat.h"              // OrionldRenderFormat
#include "orionld/types/PernotSubscription.h"               // PernotSubscription
#include "orionld/common/orionldState.h"                    // orionldState, pernotSubCache
#include "orionld/types/OrionldGeoInfo.h"                   // OrionldGeoInfo
#include "orionld/mongoc/mongocEntitiesQuery2.h"            // mongocEntitiesQuery2
#include "orionld/dbModel/dbModelToApiEntity.h"             // dbModelToApiEntity2
#include "orionld/pernot/pernotSend.h"                      // pernotSend
#include "orionld/pernot/pernotTreat.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiEntities - FIXME: Move to orionld/dbModel
//
KjNode* dbModelToApiEntities(KjNode* dbEntityArray, bool sysAttrs, OrionldRenderFormat renderFormat, char* lang)
{
  KjNode* apiEntityArray = kjArray(orionldState.kjsonP, NULL);

  for (KjNode* dbEntityP = dbEntityArray->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
  {
    OrionldProblemDetails pd;
    KjNode*               apiEntityP = dbModelToApiEntity2(dbEntityP, sysAttrs, renderFormat, lang, true, &pd);

    if (apiEntityP == NULL)
      LM_E(("dbModelToApiEntity: %s: %s", pd.title, pd.detail));
    else
      kjChildAdd(apiEntityArray, apiEntityP);
  }

  return apiEntityArray;
}



// -----------------------------------------------------------------------------
//
// pernotTreat -
//
// New Fields for Pernot Subscription:
// - pageSize for pagination  - when too many matches
// - maxPages                 - to cap the total number of paginated requests
// - local=true               - only local entities (might be the default setting)
// - totalNotifationMsgsSent  - stats: how many notifications (including paginated messages) have been sent
// - timesNoMatch             - stats: how many times did the query yield no results?
//
static void* pernotTreat(void* vP)
{
  PernotSubscription* subP = (PernotSubscription*) vP;
  bool                ok   = true;
  char                kallocBuffer[2048];

  LM_T(LmtPernotFlush, ("In thread for one Periodic Notification Subscription (%s, %f)", subP->subscriptionId, subP->lastNotificationTime));

  // 01. Initialize kalloc/kjson
  // 02. Initialize orionldState
  // 03. Prepare orionldState for the call to mongocEntitiesQuery2():
  //     - orionldState.uriParams.offset  (URL parameter)
  //     - orionldState.uriParams.limit   (URL parameter)
  //     - orionldState.uriParams.count   (URL parameter)
  //     - orionldState.tenantP           (HTTP header)
  // 04. KjNode*         entitySelectorP  (should be created when sub added to cache)
  // 05. KjNode*         attrsArray       (should be created when sub added to cache)
  // 06. QNode*          qTree            (should be created when sub added to cache)
  // 07. OrionldGeoInfo* geoInfoP         (should be created when sub added to cache)
  // 08. char*           lang             (get it from subP->kjTreeP - cache pointer in PernotSubscription)
  // 09. int64_t*        countP
  // 10. Perform the query/queries
  //     - First query with count
  //     - After that, a loop for pagination, if need be
  //    

  LM_T(LmtPernotLoop, ("%s: lastNotificationTime: %f", subP->subscriptionId, subP->lastNotificationTime));

  orionldStateInit(NULL);
  bzero(kallocBuffer, sizeof(kallocBuffer));
  kaBufferInit(&kalloc, kallocBuffer, sizeof(kallocBuffer), 8 * 1024, NULL, "Pernot KAlloc buffer");
  orionldState.kjsonP = kjBufferCreate(&kjson, &kalloc);
  
  LM_T(LmtPernot, ("Creating the query for pernot-subscription %s", subP->subscriptionId));
  int64_t          count      = 0;
  KjNode*          dbEntityArray;
  KjNode*          apiEntityArray;

  orionldState.uriParams.offset = 0;
  orionldState.uriParams.limit  = 20;     // Or: set in subscription
  orionldState.uriParams.count  = true;   // Need the count to be able to paginate
  orionldState.tenantP          = subP->tenantP;

  dbEntityArray = mongocEntitiesQuery2(subP->eSelector, subP->attrsSelector, subP->qSelector, NULL, subP->lang, &count);

  if ((dbEntityArray == NULL) || (count == 0))
  {
    LM_T(LmtPernotFlush, ("mongocEntitiesQuery2 found no matches (noMatch was %d)", subP->noMatch));
    subP->noMatch += 1;
    subP->dirty   += 1;
    goto done;
  }

  apiEntityArray = dbModelToApiEntities(dbEntityArray, subP->sysAttrs, subP->renderFormat, subP->lang);
  kjTreeLog(apiEntityArray, "apiEntityArray", LmtPernot);

  if (pernotSend(subP, apiEntityArray) == false)
    ok = false;
  else
  {
    int entitiesSent = MIN(count, orionldState.uriParams.limit);
    if (entitiesSent < count)
    {
      orionldState.uriParams.count = false;
      while (entitiesSent < count)
      {
        orionldState.uriParams.offset = entitiesSent;

        dbEntityArray  = mongocEntitiesQuery2(subP->eSelector, subP->attrsSelector, subP->qSelector, NULL, subP->lang, NULL);
        apiEntityArray = dbModelToApiEntities(dbEntityArray, subP->sysAttrs, subP->renderFormat, subP->lang);

        if (pernotSend(subP, apiEntityArray) == false)
        {
          ok = false;
          break;
        }

        entitiesSent += orionldState.uriParams.limit;
      }
    }
  }

  //
  // Timestamps and Status
  //
  if (ok == true)
  {
    LM_T(LmtPernot, ("Successful Periodic Notification"));
    subP->lastSuccessTime   = subP->lastNotificationTime;
    subP->consecutiveErrors = 0;
  }
  else
  {
    LM_T(LmtPernot, ("Failed Periodic Notification"));
    subP->lastFailureTime    = subP->lastNotificationTime;
    subP->notificationErrors += 1;
    subP->consecutiveErrors  += 1;

    if (subP->consecutiveErrors >= 3)
    {
      subP->state = SubErroneous;
      LM_W(("%s: 3 consecutive errors - setting the subscription in Error state", subP->subscriptionId));
    }
  }

done:
  subP->notificationAttempts += 1;  // timesSent
  kaBufferReset(&orionldState.kalloc, true);
  pthread_exit(0);

  return NULL;
}



// -----------------------------------------------------------------------------
//
// pernotTreatStart -
//
void pernotTreatStart(PernotSubscription* subP)
{
  pthread_t          tid;

  LM_T(LmtPernotLoop, ("Starting thread for one Periodic Notification Subscription (%s, %f)", subP->subscriptionId, subP->lastNotificationTime));
  
  pthread_create(&tid, NULL, pernotTreat, subP);

  // It's OK to lose the thread ID ... I think ...
}
