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
#include "kbase/kTime.h"                                         // kTimeGet
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "cache/CachedSubscription.h"                            // CachedSubscription

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldPatchApply.h"                    // orionldPatchApply
#include "orionld/types/OrionldAlteration.h"                     // OrionldAlteration, orionldAlterationType
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/notifications/subCacheAlterationMatch.h"       // subCacheAlterationMatch
#include "orionld/notifications/notificationSend.h"              // notificationSend
#include "orionld/notifications/notificationSuccess.h"           // notificationSuccess
#include "orionld/notifications/notificationFailure.h"           // notificationFailure
#include "orionld/notifications/orionldAlterationsTreat.h"       // Own interface



// -----------------------------------------------------------------------------
//
// NotificationPending -
//
typedef struct NotificationPending
{
  CachedSubscription*          subP;
  int                          fd;
  struct NotificationPending*  next;
} NotificationPending;



// -----------------------------------------------------------------------------
//
// notificationResponseTreat -
//
static int notificationResponseTreat(NotificationPending* npP, double timestamp)
{
  char buf[2048];  // should be enough for the HTTP headers ...
  int  nb = read(npP->fd, buf, sizeof(buf) - 1);

  if (nb == -1)
  {
    notificationFailure(npP->subP, "Unable to read from notification endpoint", timestamp);
    LM_E(("Internal Error (unable to read response for notification)"));
    return -1;
  }

  char* endOfFirstLine = strchr(buf, '\r');

  if (endOfFirstLine == NULL)
  {
    notificationFailure(npP->subP, "Invalid response from notification endpoint", timestamp);
    LM_E(("Internal Error (unable to find end of first line from notification endpoint: %s)", strerror(errno)));
    return -1;
  }

  *endOfFirstLine = 0;

  //
  // Reading the rest of the message, using select
  //
  while ((nb = read(npP->fd, buf, sizeof(buf) - 1)) > 0)
  {
    buf[nb] = 0;
  }


  return 0;
}



// -----------------------------------------------------------------------------
//
// orionldAlterationName - FIXME: Move to orionld/types/OrionldAlteration.cpp
//
const char* orionldAlterationName(OrionldAlterationType alterationType)
{
  switch (alterationType)
  {
  case EntityCreated:                return "EntityCreated";
  case EntityDeleted:                return "EntityDeleted";
  case EntityModified:               return "EntityModified";
  case AttributeAdded:               return "AttributeAdded";
  case AttributeDeleted:             return "AttributeDeleted";
  case AttributeValueChanged:        return "AttributeValueChanged";
  case AttributeMetadataChanged:     return "AttributeMetadataChanged";
  case AttributeModifiedAtChanged:   return "AttributeModifiedAtChanged";
  }

  return "Unclear Reason";
}



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
  // ----------------------------  <DEBUG>
  int alterations = 0;
  for (OrionldAlteration* aP = altList; aP != NULL; aP = aP->next)
  {
    LM(("ALT: Alteration %d:", alterations));
    LM(("ALT:   Entity Id:    %s", aP->entityId));
    LM(("ALT:   Entity Type:  %s", aP->entityType));
    LM(("ALT:   Attributes:   %d", aP->alteredAttributes));

    for (int ix = 0; ix < aP->alteredAttributes; ix++)
    {
      LM(("ALT:   Attribute        %s", aP->alteredAttributeV[ix].attrName));
      LM(("ALT:   Alteration Type: %s", orionldAlterationType(aP->alteredAttributeV[ix].alterationType)));
    }
    ++alterations;
  }
  LM(("ALT: %d Alterations present", alterations));
  // ----------------------------  </DEBUG>

  OrionldAlterationMatch* matchList;
  int                     matches;

  matchList = subCacheAlterationMatch(altList, &matches);

  if (matchList == NULL)
    return;


  //
  // The matches are ordered, grouped by subscriptionId
  // So, before notificationSend is called, the first "group" is removed from the matchList
  // to create a new list and that new list is sent to notificationSend
  //
  NotificationPending* notificationList = NULL;

#ifdef DEBUG
  int ix = 1;
  LM(("%d Matching subscriptions:", matches));
  for (OrionldAlterationMatch* matchP = matchList; matchP != NULL; matchP = matchP->next)
  {
    if (matchP->altAttrP != NULL)
      LM(("o %d/%d Subscription '%s', due to '%s'", ix, matches, matchP->subP->subscriptionId, orionldAlterationName(matchP->altAttrP->alterationType)));
    else
      LM(("o %d/%d Subscription '%s'", ix, matches, matchP->subP->subscriptionId));
    ++ix;
  }
#endif


  //
  // Applying the PATCH, if any
  //
  if (altList->dbEntityP != NULL)
  {
    altList->patchedEntity = dbModelToApiEntity(altList->dbEntityP, false, altList->entityId);  // No sysAttrs options for subscriptions?

    for (KjNode* patchP = altList->patchTree->value.firstChildP; patchP != NULL; patchP = patchP->next)
    {
      orionldPatchApply(altList->patchedEntity, patchP);
    }
  }

  //
  // Timestamp for sending of notification - for stats in subscriptions
  //
  struct timespec  notificationTime;
  double           notificationTimeAsFloat;
  kTimeGet(&notificationTime);
  notificationTimeAsFloat = notificationTime.tv_sec + ((double) notificationTime.tv_nsec) / 1000000000;

  for (OrionldAlterationMatch* mAltP = matchList; mAltP != NULL; mAltP = mAltP->next)
  {
    LM(("Sending notification for OrionldAlterationMatch at %p (patchedEntity at %p)", mAltP, mAltP->altP->patchedEntity));
    //
    // Neeed to clone the "patched entity", there might be more notifications wit the same "patched entity"
    //
    int fd = notificationSend(mAltP, notificationTimeAsFloat);
    if (fd != -1)
    {
      NotificationPending* npP = (NotificationPending*) kaAlloc(&orionldState.kalloc, sizeof(NotificationPending));

      npP->subP = mAltP->subP;
      npP->fd   = fd;
      npP->next = notificationList;
      notificationList = npP;
    }
  }

  //
  // Awaiting responses and updating subscriptions accordingly (in sub cache)
  //
  int             fds;
  fd_set          rFds;
  int             fdMax      = 0;
  int             startTime  = time(NULL);
  bool            timeout    = false;

  while (timeout == false)
  {
    //
    // Set File Descriptors for the select
    //
    NotificationPending* npP = notificationList;

    fdMax = 0;
    FD_ZERO(&rFds);
    while (npP != NULL)
    {
      if (npP->fd != -1)
      {
        FD_SET(npP->fd, &rFds);
        fdMax = MAX(fdMax, npP->fd);
      }
      npP = npP->next;
    }

    if (fdMax == 0)
      break;

    struct timeval  tv = { 1, 0 };
    fds = select(fdMax + 1, &rFds, NULL, NULL, &tv);
    if (fds == -1)
    {
      if (errno != EINTR)
        LM_X(1, ("select error: %s\n", strerror(errno)));
    }

    while (fds > 0)
    {
      npP = notificationList;
      while (npP != NULL)
      {
        if ((npP->fd != -1) && (FD_ISSET(npP->fd, &rFds)))
        {
          if (notificationResponseTreat(npP, notificationTimeAsFloat) == 0)
            notificationSuccess(npP->subP, notificationTimeAsFloat);

          close(npP->fd);  // OR: close socket inside responseTreat?
          npP->fd = -1;
          --fds;
          break;
        }

        npP = npP->next;
      }

      if (time(NULL) > startTime + 4)
      {
        timeout = true;
        break;
      }
    }
  }

  NotificationPending* npP = notificationList;
  while (npP != NULL)
  {
    if (npP->fd != -1)
    {
      // No response
      if (strncmp(npP->subP->protocol, "mqtt", 4) != 0)
      {
        notificationFailure(npP->subP, "Timeout awaiting response from notification endpoint", notificationTimeAsFloat);
        close(npP->fd);
        npP->fd = -1;
      }
    }

    npP = npP->next;
  }
}
