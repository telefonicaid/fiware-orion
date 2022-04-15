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
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "cache/subCache.h"                                      // CachedSubscription, subCacheMatch, subscriptionFailure, subscriptionSuccess

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldPatchApply.h"                    // orionldPatchApply
#include "orionld/types/OrionldAlteration.h"                     // OrionldAlteration, orionldAlterationType
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/notifications/subCacheAlterationMatch.h"       // subCacheAlterationMatch
#include "orionld/notifications/notificationSend.h"              // notificationSend
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
    subscriptionFailure(npP->subP, "Unable to read from notification endpoint", timestamp);
    LM_E(("Internal Error (unable to read response for notification)"));
    return -1;
  }

  LM_TMP(("NFY: Read %d bytes of notification: %s", nb, buf));

  char* endOfFirstLine = strchr(buf, '\r');

  if (endOfFirstLine == NULL)
  {
    subscriptionFailure(npP->subP, "Invalid response from notification endpoint", timestamp);
    LM_E(("Internal Error (unable to find end of first line from notification endpoint: %s)", strerror(errno)));
    return -1;
  }

  *endOfFirstLine = 0;
  LM_TMP(("NFY: First line: '%s'", buf));
  LM_TMP(("NFY: Rest: '%s'", &endOfFirstLine[1]));

  //
  // Reading the rest of the message, using select
  //
  while ((nb = read(npP->fd, buf, sizeof(buf) - 1)) > 0)
  {
    buf[nb] = 0;
    LM_TMP(("NFY: Read a chunk of %d bytes: %s", nb, buf));
  }


  return 0;
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
  // <DEBUG>
  int alterations = 0;
  for (OrionldAlteration* aP = altList; aP != NULL; aP = aP->next)
  {
    ++alterations;
  }
  LM_TMP(("%d Alterations present", alterations));
  // </DEBUG>

  OrionldAlterationMatch* matchList;
  int                     matches;

  matchList = subCacheAlterationMatch(altList, &matches);

  if (matchList == NULL)
    LM_RVE(("No matching subscriptions"));


  //
  // The matches are ordered, grouped by subscriptionId
  // So, before notificationSend is called, the first "group" is removed from the matchList
  // to create a new list and that new list is sent to notificationSend
  //
  NotificationPending* notificationList = NULL;

  // <DEBUG>
  int ix = 1;
  LM_TMP(("XX: %d Matching subscriptions:", matches));
  for (OrionldAlterationMatch* matchP = matchList; matchP != NULL; matchP = matchP->next)
  {
    LM_TMP(("XX:   %d/%d Subscription '%s'", ix, matches, matchP->subP->subscriptionId));
  }
  // </DEBUG>


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

  //
  // Sorting matches into groups and sending notifications
  //
  while (matchList != NULL)
  {
    // Get the group
    OrionldAlterationMatch* groupList = matchList;
    OrionldAlterationMatch* prev      = matchList;
    LM_TMP(("XX: groupList at %p (next at %p)", groupList, groupList->next));
    for (OrionldAlterationMatch* matchP = groupList->next; matchP != NULL; matchP = matchP->next)
    {
      LM_TMP(("XX: Subscription '%s' vs '%s'", matchP->subP->subscriptionId, groupList->subP->subscriptionId));
      if (matchP->subP != groupList->subP)
      {
        matchList  = matchP;  // matchList pointing to the first non-matching
        prev->next = NULL;    // Ending the 'groupList'
        LM_TMP(("Breaking match loop for sub '%s' - next is %p", groupList->subP->subscriptionId, matchList));
        break;
      }

      prev = matchP;
      if (matchP->next == NULL)
        matchList = NULL;
    }

    // <DEBUG>
    int groupMembers = 0;
    LM_TMP(("XX: Alterations matching sub '%s':", groupList->subP->subscriptionId));
    for (OrionldAlterationMatch* matchP = groupList; matchP != NULL; matchP = matchP->next)
    {
      ++groupMembers;
      if (matchP->altAttrP)
        LM_TMP(("XX  - %s", orionldAlterationType(matchP->altAttrP->alterationType)));
    }
    // </DEBUG>

    int fd = notificationSend(groupList, notificationTimeAsFloat);
    if (fd != -1)
    {
      NotificationPending* npP = (NotificationPending*) kaAlloc(&orionldState.kalloc, sizeof(NotificationPending));

      npP->subP = groupList->subP;
      npP->fd   = fd;
      npP->next = notificationList;
      notificationList = npP;
    }

    if (groupList == matchList)
      break;
  }


  //
  // Awaiting responses and updating subscriptions accordingly (in sub cache)
  //
  int             fds;
  fd_set          rFds;
  int             fdMax      = 0;
  int             startTime  = time(NULL);
  bool            timeout    = false;

  LM_TMP(("Awaiting responses and updating cached subscriptions accordingly"));
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

    LM_TMP(("Awaiting responses to the notifications (fdMax: %d)", fdMax));
    struct timeval  tv = { 1, 0 };
    fds = select(fdMax + 1, &rFds, NULL, NULL, &tv);
    LM_TMP(("Got %d from select", fds));
    if (fds == -1)
    {
      if (errno != EINTR)
        LM_X(1, ("select error: %s\n", strerror(errno)));
    }

    while (fds > 0)
    {
      LM_TMP(("Got %d notification response(s)", fds));
      npP = notificationList;
      while (npP != NULL)
      {
        if ((npP->fd != -1) && (FD_ISSET(npP->fd, &rFds)))
        {
          LM_TMP(("Detected a notification response on fd %d", npP->fd));
          if (notificationResponseTreat(npP, notificationTimeAsFloat) == 0)
            subscriptionSuccess(npP->subP, notificationTimeAsFloat);

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

  LM_TMP(("Mark all subs for unanswered notifications as timed out if npP->fd != -1"));
  NotificationPending* npP = notificationList;
  while (npP != NULL)
  {
    if (npP->fd != -1)
    {
      // No response
      subscriptionFailure(npP->subP, "Timeout awaiting response from notification endpoint", notificationTimeAsFloat);
      close(npP->fd);
      npP->fd = -1;
    }

    npP = npP->next;
  }
}
