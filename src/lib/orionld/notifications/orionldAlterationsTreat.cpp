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
#include <curl/curl.h>                                           // CURL

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjRender.h"                                      // kjFastRender
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
  CURL*                        curlHandleP;
  struct NotificationPending*  next;
} NotificationPending;



// -----------------------------------------------------------------------------
//
// notificationResponseTreat -
//
static int notificationResponseTreat(NotificationPending* npP, double notificationTime)
{
  char buf[2048];  // should be enough for the HTTP headers ...
  int  nb = read(npP->fd, buf, sizeof(buf) - 1);

  if (nb == -1)
  {
    notificationFailure(npP->subP, "Unable to read from notification endpoint", notificationTime);
    LM_E(("Internal Error (unable to read response for notification)"));
    return -1;
  }

  char* endOfFirstLine = strchr(buf, '\r');

  if (endOfFirstLine == NULL)
  {
    notificationFailure(npP->subP, "Invalid response from notification endpoint", notificationTime);
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
// notificationLookupByCurlHandle -
//
static NotificationPending* notificationLookupByCurlHandle(NotificationPending* notificationList, CURL* curlHandleP)
{
  for (NotificationPending* npP = notificationList; npP != NULL; npP = npP->next)
  {
    if (npP->curlHandleP == curlHandleP)
      return npP;
  }

  return NULL;
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
#if 0
  // <DEBUG>
  int alterations = 0;
  for (OrionldAlteration* aP = altList; aP != NULL; aP = aP->next)
  {
    LM_T(LmtAlt, (" Alteration %d:", alterations));
    LM_T(LmtAlt, ("   Entity In:      %p", aP->inEntityP));
    LM_T(LmtAlt, ("   CompleteEntity: %p", aP->finalApiEntityP));
    LM_T(LmtAlt, ("   Entity Id:      %s", aP->entityId));
    LM_T(LmtAlt, ("   Entity Type:    %s", aP->entityType));
    LM_T(LmtAlt, ("   Attributes:     %d", aP->alteredAttributes));

    for (int ix = 0; ix < aP->alteredAttributes; ix++)
    {
      LM_T(LmtAlt, ("   Attribute        %s", aP->alteredAttributeV[ix].attrName));
      LM_T(LmtAlt, ("   Alteration Type: %s", orionldAlterationType(aP->alteredAttributeV[ix].alterationType)));
    }

    kjTreeLog(aP->inEntityP, "ALT:   inEntityP", LmtAlt);
    ++alterations;
  }
  LM_T(LmtAlt, (" %d Alterations present", alterations));
  // </DEBUG>
#endif

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
  LM(("%d items in matchList:", matches));
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
  // FIXME:
  //   The original db entity is needed only to apply the patch.
  //   Would be better to apply the patch before so we would not need "altList->dbEntityP"
  //

#if 1
  //
  // Only orionldPatchEntity2 uses this ... Right?
  // Instead of having this function create finalApiEntityP, it should be done by the service routine
  // And, after that, altList->dbEntityP is no longer needed
  //
  // FIXME: Make orionldPatchEntity2 create the finalApiEntity itself !!!
  //
  if ((altList->dbEntityP != NULL) && (altList->finalApiEntityP == NULL))
  {
    altList->finalApiEntityP = dbModelToApiEntity(altList->dbEntityP, false, altList->entityId);  // No sysAttrs options for subscriptions?

    for (KjNode* patchP = altList->inEntityP->value.firstChildP; patchP != NULL; patchP = patchP->next)
    {
      orionldPatchApply(altList->finalApiEntityP, patchP);
    }
  }
#endif

  //
  // Timestamp for sending of notification - for stats in subscriptions
  // The request time is used instead of calling the system to ask for a new time
  //
  double notificationTime = orionldState.requestTime;

  while (matchList != NULL)
  {
    //
    // 1. Extract first match from matchList, call it matchHead
    // 2. Extract all subsequent matches from matchList - add them to matchHead
    // 3. Call notificationSend, which will combine all of them into one single notification
    //
    // The matches come in subscription order, so, we just need to break after the first non-same subscription
    //
    OrionldAlterationMatch* matchHead = matchList;

    matchList = matchList->next;
    matchHead->next = NULL;  // The matchHead-list is now NULL-terminated and separated from matchList

    while ((matchList != NULL) && (matchList->subP == matchHead->subP))
    {
      OrionldAlterationMatch* current = matchList;

      matchList     = matchList->next;
      current->next = matchHead;
      matchHead     = current;
    }

    CURL* curlHandleP;
    int   fd = notificationSend(matchHead, notificationTime, &curlHandleP);  // curl handle as output param?
    if (fd != -1)
    {
      NotificationPending* npP = (NotificationPending*) kaAlloc(&orionldState.kalloc, sizeof(NotificationPending));

      npP->subP        = matchHead->subP;
      npP->fd          = fd;
      npP->curlHandleP = curlHandleP;

      npP->next        = notificationList;
      notificationList = npP;
    }
  }

  // Start HTTPS notifications
  bool curlError = false;
  if (orionldState.multiP != NULL)
  {
    int        activeNotifications;
    CURLMcode  cm;

    LM(("Starting HTTPS notifications"));
    cm = curl_multi_perform(orionldState.multiP, &activeNotifications);
    if (cm != 0)
    {
      LM_E(("curl_multi_perform: error %d", cm));
      curlError = true;
    }
  }

  //
  // Await HTTP responses and update subscriptions accordingly (in sub cache)
  //
  int             fds;
  fd_set          rFds;
  int             fdMax      = 0;
  int             startTime  = time(NULL);
  bool            timeout    = false;

  while (timeout == false)
  {
    NotificationPending* npP = notificationList;

    //
    // Set File Descriptors for the select
    //
    fdMax = 0;
    FD_ZERO(&rFds);
    while (npP != NULL)
    {
      if (npP->fd >= 0)  // Only HTTP notifications
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
        if ((npP->fd >= 0) && (FD_ISSET(npP->fd, &rFds)))
        {
          if (notificationResponseTreat(npP, notificationTime) == 0)
            notificationSuccess(npP->subP, notificationTime);

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
    if (npP->fd >= 0)
    {
      // No response
      if (strncmp(npP->subP->protocolString, "mqtt", 4) != 0)
      {
        notificationFailure(npP->subP, "Timeout awaiting response from notification endpoint", notificationTime);
        close(npP->fd);
        npP->fd = -1;
      }
    }

    npP = npP->next;
  }

  if (curlError == true)
  {
    LM_E(("Something went wrong with the HTTPS notifications"));
    return;
  }

  // Await HTTPS notification responses
  if (orionldState.multiP != NULL)
  {
    int activeNotifications = 1;

    while (activeNotifications != 0)
    {
      CURLMcode  cm;

      cm = curl_multi_perform(orionldState.multiP, &activeNotifications);
      if (cm != 0)
      {
        LM_E(("curl_multi_perform: error %d", cm));
        curlError = true;
        break;
      }
      else
        LM(("%d HTTPS notifications are still active", activeNotifications));

      if (activeNotifications > 0)
      {
        //
        // The curl library of the UBI base image doesn't have "curl_multi_poll".
        // Using curl_multi_wait instead.
        // For now, at least
        //
        LM(("Calling curl_multi_wait"));
        cm = curl_multi_wait(orionldState.multiP, NULL, 0, 1000, NULL);
        if (cm != 0)
          LM_E(("curl_multi_poll error %d", cm));
      }
    }

    // Read the results of the notifications
    CURLMsg* msgP;
    int      msgsLeft;

    LM(("Reading out the results of the notifications"));
    while ((msgP = curl_multi_info_read(orionldState.multiP, &msgsLeft)) != NULL)
    {
      if (msgP->msg != CURLMSG_DONE)
        continue;

      NotificationPending* npP = notificationLookupByCurlHandle(notificationList, msgP->easy_handle);
      LM(("Notification Host: '%s'", npP->subP->ip));
      LM(("Notification Result for subscription '%s': CURLcode %d (%s)", npP->subP->subscriptionId, msgP->data.result, curl_easy_strerror(msgP->data.result)));
      LM(("Update Counters for subscription '%s'", npP->subP->subscriptionId));

      if (msgP->data.result == 0)
        notificationSuccess(npP->subP, notificationTime);
      else
      {
        char errorString[512];

        snprintf(errorString, sizeof(errorString), "CURL Error %d: %s", msgP->data.result, curl_easy_strerror(msgP->data.result));
        notificationFailure(npP->subP, errorString, notificationTime);
      }
    }
  }
}
