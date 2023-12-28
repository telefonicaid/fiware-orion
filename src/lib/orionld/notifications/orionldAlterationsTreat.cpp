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

#include "logMsg/logMsg.h"                                       // LM_*, lmTraceIsSet

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
  bool                         used;
  struct NotificationPending*  next;
} NotificationPending;



// -----------------------------------------------------------------------------
//
// statusCodeExtract -
//
static int statusCodeExtract(char* startLine)
{
  char* space = strchr(startLine, ' ');

  if (space == NULL)
    return -1;

  return atoi(&space[1]);
}



// -----------------------------------------------------------------------------
//
// readWithTimeout - FIXME: to its own module under orionld/common
//
int readWithTimeout(int fd, char* buf, int bufLen, int tmoSecs, int tmoMicroSecs)
{
  while (1)  // "try-again" if EINTR, otherwise, either return error or finish
  {
    int            fds;
    fd_set         rFds;
    struct timeval tv    = { tmoSecs, tmoMicroSecs };
    int            fdMax = fd;

    FD_ZERO(&rFds);
    FD_SET(fd, &rFds);

    fds = select(fdMax + 1, &rFds, NULL, NULL, &tv);
    if (fds == -1)
    {
      if (errno == EINTR)
        continue;

      LM_E(("select error: %s", strerror(errno)));
      return -1;
    }
    else if (fds == 0)
      return 0;

    if (!FD_ISSET(fd, &rFds))
      return -1;  // This can't happen ...

    break;
  }

  return read(fd, buf, bufLen);
}



// -----------------------------------------------------------------------------
//
// notificationResponseRead - FIXME: to its own module
//
bool notificationResponseRead
(
  NotificationPending* npP,
  char*                buf,
  int                  bufLen,
  int*                 httpStatusCodeP,
  int*                 contentLengthP,
  char**               headersP,
  char**               bodyP,
  double               notificationTime
)
{
  //
  // 1. Do an initial read, with buf and bufLen
  // 2. Make sure we have headerBodyDelimiter
  //    - if 100% of body, we're done
  //    - else, read more
  //      - if timeout, return error
  //
  int      contentLen  = -1;
  int      httpStatus  = -1;
  char*    headers     = NULL;
  char*    body        = NULL;
  ssize_t  bytesRead   = 0;

  //
  // Initial read
  //
  bytesRead = readWithTimeout(npP->fd, buf, bufLen - 1, 0, 100000);
  if (bytesRead <= 0)
  {
    notificationFailure(npP->subP, "Unable to read from notification endpoint", notificationTime);
    LM_E(("Internal Error (%s: unable to read response for notification on fd %d)", npP->subP->subscriptionId, npP->fd));
    return false;
  }
  buf[bytesRead] = 0;

  //
  // Find the Header/Body delimiter
  // Read more if necessary
  //
  char* headerBodyDelimiterP = strstr(buf, "\r\n\r\n");

  if (headerBodyDelimiterP == NULL)
    headerBodyDelimiterP = strstr(buf, "\n\n");

  if (headerBodyDelimiterP == NULL)
  {
    // Read mode
    int nb = readWithTimeout(npP->fd, &buf[bytesRead], bufLen - bytesRead, 0, 100000);  // 100 millisecond timeout
    if (nb == 0)
    {
      LM_W(("%s: the read of notification response timed out", npP->subP->subscriptionId));
      notificationFailure(npP->subP, "timeout reading the notification response", notificationTime);
      return false;
    }
    else if (nb == -1)
    {
      char errorString[512];
      snprintf(errorString, sizeof(errorString), "error reading notification response: %s", strerror(errno));
      LM_E(("%s: %s", npP->subP->subscriptionId, errorString));
      notificationFailure(npP->subP, errorString, notificationTime);
      return false;
    }

    bytesRead += nb;

    // Try to find the Header/Body delimiter again
    headerBodyDelimiterP = strstr(buf, "\r\n\r\n");
    if (headerBodyDelimiterP == NULL)
      headerBodyDelimiterP = strstr(buf, "\n\n");

    if (headerBodyDelimiterP == NULL)
    {
      LM_W(("Can't find the Headers/Body delimiter"));
      notificationFailure(npP->subP, "Can't find the Headers/Body delimiter", notificationTime);
      return false;
    }
  }

  // Zero delimit the Header/Body Delimiter
  *headerBodyDelimiterP = 0;

  // Step over all \r | \n  and assign the bodyP
  ++headerBodyDelimiterP;

  while ((*headerBodyDelimiterP == '\r') || (*headerBodyDelimiterP == '\n'))
  {
    ++headerBodyDelimiterP;
  }

  body = headerBodyDelimiterP;

  //
  // Find the end of the Start-Line and zero-terminate it
  // Make headersP reference what comes after it
  //
  char* cP = strstr(buf, "\r\n");
  if (cP == NULL)
  {
    cP = strstr(buf, "\n");
    if (cP != NULL)
    {
      *cP = 0;
      ++cP;
    }
  }
  else
  {
    *cP = 0;
    cP += 2;  // Jump over the '\r' and '\n'
  }

  if (cP == NULL)
  {
    LM_W(("%s: Can't find the end of the Start-Line", npP->subP->subscriptionId));
    notificationFailure(npP->subP, "Can't find the end of the Start-Line", notificationTime);
    return false;
  }

  headers = cP;
  LM_T(LmtNotificationMsg, ("%s: notification response Start-Line:   '%s'", npP->subP->subscriptionId, buf));
  LM_T(LmtNotificationMsg, ("%s: notification response HTTP Headers: '%s'", npP->subP->subscriptionId, headers));
  LM_T(LmtNotificationMsg, ("%s: notification response body so far: '%s'", body));


  //
  // Extract the Status Code from the first line
  //
  httpStatus = statusCodeExtract(buf);

  // Extract the Content-Length (if not part of the headers and status code is not 204, it's considered an error)
  char* contentLenP = strstr(headers, "Content-Length:");
  if (contentLenP == NULL)
  {
    // Error if not 204
    if (httpStatus != 204)
    {
      LM_W(("Content-Length not found but the status code (%d) is not a 204", httpStatus));
      notificationFailure(npP->subP, "Content-Length not found but the status code is not a 204", notificationTime);
      return false;
    }
    contentLen = 0;
  }
  else
    contentLen = atoi(&contentLenP[16]);

  LM_T(LmtNotificationMsg, ("%s: Content-Length: %d", npP->subP->subscriptionId, contentLen));


  //
  // Make sure we've read the entire body
  //
  ssize_t headersLen    = (ssize_t) headerBodyDelimiterP - (ssize_t) buf;  // Including the Start-Line
  ssize_t bodyBytesRead = bytesRead - headersLen;

  LM_T(LmtNotificationMsg, ("%s: total no of bytes read: %d", npP->subP->subscriptionId, bytesRead));
  LM_T(LmtNotificationMsg, ("%s: no of bytes of body read: %d", npP->subP->subscriptionId, bodyBytesRead));

  if (bodyBytesRead < contentLen)
  {
    //
    // We need to read more
    // Do we have room enough in 'buf'?
    // If not, we'll have to allocate more (using kaAlloc)
    //
    int bodyBytesStillToRead = contentLen - bodyBytesRead;

    if (bytesRead + bodyBytesStillToRead >= bufLen)
    {
      LM_T(LmtNotificationMsg, ("%s: must reallocate for the response body (we have %d bytes left in buffer, need %d)",
          npP->subP->subscriptionId,
          bufLen - bytesRead,
          bodyBytesStillToRead));

      char* newBody = kaAlloc(&orionldState.kalloc, contentLen + 1);
      if (newBody == NULL)
      {
        LM_E(("Unable to allocate %d bytes for notification response body", contentLen + 1));
        notificationFailure(npP->subP, "Unable to allocate buffer for notification response body", notificationTime);
        return false;
      }

      // Copy what's already read, from body to newBody
      strncpy(newBody, body, bodyBytesRead + 1);
      body = newBody;

      // Read the rest of the body
      int nb = readWithTimeout(npP->fd, &newBody[bodyBytesRead], contentLen - bodyBytesRead, 0, 100000);  // 100 millisecond timeout
      if (nb == 0)
      {
        LM_W(("The read timed out"));
        notificationFailure(npP->subP, "timeout while reading notification response", notificationTime);
        return false;
      }
      else if (nb == -1)
      {
        LM_E(("Other error reading (%s)", strerror(errno)));
        return false;
      }

      bodyBytesRead += nb;

      if (bodyBytesRead != contentLen)
      {
        LM_W(("Still not enough bytes read for the notification response body. I give up"));
        notificationFailure(npP->subP, "Unable to read the entire response", notificationTime);
        return false;
      }
    }
  }

  LM_T(LmtNotificationMsg, ("%s: entire message read", npP->subP->subscriptionId));
  *headersP        = headers;
  *bodyP           = body;
  *httpStatusCodeP = httpStatus;

  return true;
}



// -----------------------------------------------------------------------------
//
// notificationResponseTreat -
//
static void notificationResponseTreat(NotificationPending* npP, double notificationTime)
{
  char  buf[2048];  // should be enough for the HTTP headers ...
  int   contentLength  = -1;
  int   httpStatusCode = -1;
  char* body           = NULL;
  char* headers        = NULL;
  char* subId          = npP->subP->subscriptionId;

  bzero(buf, sizeof(buf));

  if (notificationResponseRead(npP, buf, sizeof(buf), &httpStatusCode, &contentLength, &headers, &body, notificationTime) == false)
  {
    // if notificationResponseRead() returns false, it has invoked notificationFailure()
    return;
  }

  if (lmTraceIsSet(LmtNotificationHeaders) == true)
  {
    char* headerP = headers;
    char* eol;
    while ((eol = strstr(headerP, "\n")) != NULL)
    {
      *eol = 0;
      LM_T(LmtNotificationHeaders, ("%s: Notification Response HTTP Header: '%s'", subId, headerP));
      headerP = &eol[1];
    }

    LM_T(LmtNotificationHeaders, ("%s: Notification Response HTTP Header: '%s'", subId, headerP));
  }

  LM_T(LmtNotificationBody, ("%s: Notification Response Body: '%s'", subId, body));

  //
  // Any 2xx response is considered OK
  //
  if (httpStatusCode == -1)
  {
    notificationFailure(npP->subP, "HTTP Start-Line of notification response not found", notificationTime);
    LM_E(("Internal Error (%s:  HTTP Start-Line of notification response not found)", subId));
  }
  else if ((httpStatusCode < 200) || (httpStatusCode >= 300))
  {
    notificationFailure(npP->subP, "non 2xx response to notification", notificationTime);
    LM_E(("Internal Error (%s: non 2xx response (%d) to notification on fd %d)", subId, httpStatusCode, npP->fd));
  }
  else
    notificationSuccess(npP->subP, notificationTime);
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
    if (npP->used == true)
      continue;

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
  // <DEBUG>
  if (lmTraceIsSet(LmtAlt))
  {
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

      // kjTreeLog(aP->inEntityP, "ALT:   inEntityP", LmtAlt);  // outdeffed
      ++alterations;
    }
    LM_T(LmtAlt, (" %d Alterations present", alterations));
  }
  // </DEBUG>

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

  if (lmTraceIsSet(LmtAlt) == true)
  {
    int ix = 1;
    LM_T(LmtAlt, ("%d items in matchList:", matches));
    for (OrionldAlterationMatch* matchP = matchList; matchP != NULL; matchP = matchP->next)
    {
      if (matchP->altAttrP != NULL)
        LM_T(LmtAlt, ("o %d/%d Subscription '%s', due to '%s'",
                      ix,
                      matches,
                      matchP->subP->subscriptionId,
                      orionldAlterationName(matchP->altAttrP->alterationType)));
      else
        LM_T(LmtAlt, ("o %d/%d Subscription '%s'", ix, matches, matchP->subP->subscriptionId));

      ++ix;
    }
  }

  //
  // Applying the PATCH, if any
  //
  // FIXME:
  //   The original db entity is needed only to apply the patch.
  //   Would be better to apply the patch before so we would not need "altList->dbEntityP"
  //


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

    int patchNo = 0;
    for (KjNode* patchP = altList->inEntityP->value.firstChildP; patchP != NULL; patchP = patchP->next)
    {
      ++patchNo;
      orionldPatchApply(altList->finalApiEntityP, patchP, true);
    }
  }

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

    CURL* curlHandleP = NULL;
    int   fd          = notificationSend(matchHead, notificationTime, &curlHandleP);  // curl handle as output param?

    //
    // -1 is ERROR, -2 is OK for HTTPS, anything else is a file descriptor to read from, except if MQTT.
    // For MQTT, the notificationSuccess/Failure is already taken care of by mqttNotify and nothing to read.
    // Should not be in this list.
    //
    if ((fd != -1) && (matchHead->subP->protocol != MQTT))
    {
      NotificationPending* npP = (NotificationPending*) kaAlloc(&orionldState.kalloc, sizeof(NotificationPending));

      npP->subP        = matchHead->subP;
      npP->fd          = fd;
      npP->curlHandleP = curlHandleP;
      npP->used        = false;
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

    LM_T(LmtNotificationSend, ("Starting HTTPS notifications"));
    cm = curl_multi_perform(orionldState.multiP, &activeNotifications);
    if (cm != 0)
    {
      LM_E(("Error starting HTTPS notifications: curl_multi_perform: error %d", cm));
      curlError = true;
    }
  }
  else
    LM_T(LmtNotificationSend, ("No HTTPS notifications"));

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
        if ((npP->fd >= 0) && (FD_ISSET(npP->fd, &rFds)))  // Only HTTP notifications
        {
          notificationResponseTreat(npP, notificationTime);

          close(npP->fd);
          --fds;

          npP->fd   = -1;
          npP->used = true;

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

  //
  // Find timed out HTTP notification responses
  //
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

        npP->fd   = -1;
        npP->used = true;
      }
    }

    npP = npP->next;
  }

  if (curlError == true)
  {
    LM_E(("Something went wrong with a HTTPS notification"));
    return;
  }

  //
  // Await HTTPS notification responses
  //
  if (orionldState.multiP != NULL)
  {
    LM_T(LmtNotificationSend, ("Awaiting HTTPS notification responses"));
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
        LM_T(LmtNotificationSend, ("%d HTTPS notifications are still active", activeNotifications));

      if (activeNotifications > 0)
      {
        //
        // The curl library of the UBI base image doesn't have "curl_multi_poll".
        // Using curl_multi_wait instead.
        // For now, at least
        //
        cm = curl_multi_wait(orionldState.multiP, NULL, 0, 1000, NULL);
        if (cm != 0)
          LM_E(("curl_multi_wait error %d", cm));
      }
    }

    // Read the results of the notifications
    CURLMsg* msgP;
    int      msgsLeft;

    while ((msgP = curl_multi_info_read(orionldState.multiP, &msgsLeft)) != NULL)
    {
      if (msgP->msg != CURLMSG_DONE)
        continue;

      NotificationPending* npP = notificationLookupByCurlHandle(notificationList, msgP->easy_handle);

      if (npP == NULL)
      {
        LM_W(("No 'Pending Notification' found for a curl easy handle"));
        continue;
      }

      LM_T(LmtNotificationSend, ("%s: Notification Host: '%s'", npP->subP->subscriptionId, npP->subP->ip));
      LM_T(LmtNotificationSend, ("%s: Notification Result: CURLcode %d (%s)", npP->subP->subscriptionId, msgP->data.result, curl_easy_strerror(msgP->data.result)));
      LM_T(LmtNotificationSend, ("%s: Update Counters", npP->subP->subscriptionId));

      if (msgP->data.result == 0)
      {
        uint64_t  httpResponseCode = 500;
        curl_easy_getinfo(npP->curlHandleP, CURLINFO_RESPONSE_CODE, &httpResponseCode);

        LM_T(LmtNotificationSend, ("%s: Notification Response HTTP Status: %d", npP->subP->subscriptionId, (int) httpResponseCode));

        if ((httpResponseCode >= 200) && (httpResponseCode < 300))
          notificationSuccess(npP->subP, notificationTime);
        else
        {
          char errorString[256];

          snprintf(errorString, sizeof(errorString), "Got an HTTP Status %d", (int) httpResponseCode);
          notificationFailure(npP->subP, errorString, notificationTime);
        }
      }
      else
      {
        char errorString[512];

        snprintf(errorString, sizeof(errorString), "CURL Error %d: %s", msgP->data.result, curl_easy_strerror(msgP->data.result));
        notificationFailure(npP->subP, errorString, notificationTime);
      }

      npP->used = true;
    }
  }

  for (NotificationPending* npP = notificationList; npP != NULL; npP = npP->next)
  {
    if (npP->used == true)
      continue;

    if (npP->subP->protocol == MQTT)
      continue;

    notificationFailure(npP->subP, "Notification never reached its destination", notificationTime);
  }
}
