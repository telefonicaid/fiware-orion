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
#include <sys/uio.h>                                             // iovec
#include <curl/curl.h>                                           // curl

#include "logMsg/logMsg.h"                                       // LM*
#include "cache/CachedSubscription.h"                            // CachedSubscription
#include "orionld/types/OrionldAlteration.h"                     // OrionldAlterationMatch
#include "orionld/common/orionldState.h"                         // orionldState



// -----------------------------------------------------------------------------
//
// curlDebug - from orionld/common/orionldRequestSend.cpp
//
extern int curlDebug(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr);



// -----------------------------------------------------------------------------
//
// curlSaveForLaterCleanup -
//
static void curlSaveForLaterCleanup(CURL* curlHandleP, struct curl_slist* headersP)
{
  if (orionldState.easyV == NULL)
  {
    orionldState.easyV    = (CURL**) malloc(10 * sizeof(CURL*));
    orionldState.easySize = 10;
    orionldState.easyIx   = 0;
  }
  else if (orionldState.easyIx >= orionldState.easySize)
  {
    orionldState.easySize += 10;
    CURL** easyV           = (CURL**) realloc(orionldState.easyV,  orionldState.easySize * sizeof(CURL*));;

    if (easyV == NULL)
      LM_X(1, ("Out of memory allocating %d curl easy handles", orionldState.easySize));

    orionldState.easyV = easyV;
  }

  orionldState.easyV[orionldState.easyIx] = curlHandleP;
  ++orionldState.easyIx;

  if (orionldState.curlHeadersV == NULL)
  {
    orionldState.curlHeadersV    = (struct curl_slist**) malloc(10 * sizeof(struct curl_slist*));
    orionldState.curlHeadersSize = 10;
    orionldState.curlHeadersIx   = 0;
  }
  else if (orionldState.curlHeadersIx >= orionldState.curlHeadersSize)
  {
    orionldState.curlHeadersSize += 10;
    struct curl_slist**  headers   = (struct curl_slist**) realloc(orionldState.curlHeadersV, orionldState.curlHeadersSize * sizeof(struct curl_slist*));

    if (headers == NULL)
      LM_X(1, ("Out of memory allocating %d curl header slots", orionldState.curlHeadersSize));

    orionldState.curlHeadersV = headers;
  }

  orionldState.curlHeadersV[orionldState.curlHeadersIx] = headersP;
  ++orionldState.curlHeadersIx;
}



// -----------------------------------------------------------------------------
//
// responseHeaderDebug -
//
static size_t responseHeaderDebug(char* buffer, size_t size, size_t nitems, void* userdata)
{
  char* subId = (char*) userdata;
  LM_T(LmtNotificationHeaders, ("%s: Notification Response HTTP Header: %s", subId, buffer));
  return nitems;
}



// -----------------------------------------------------------------------------
//
// notificationResponseBody -
//
static int notificationResponseBody(void* chunk, size_t size, size_t members, void* userP)
{
  size_t chunkLen = members;
  char*  chunkP   = (char*) chunk;

  LM_T(LmtNotificationBody, ("Got a chunk of notification response (%d bytes): %s", chunkLen, chunkP));

  return chunkLen;
}



// -----------------------------------------------------------------------------
//
// httpsNotify -
//
int httpsNotify(CachedSubscription* cSubP, struct iovec* ioVec, int ioVecLen, double timestamp, CURL** curlHandlePP)
{
  LM_T(LmtNotificationSend, ("%s: Protocol for HTTPS notification: %s (%d)", cSubP->subscriptionId, cSubP->protocolString, cSubP->protocol));
  LM_T(LmtNotificationSend, ("%s: IP for HTTPS notification: %s", cSubP->subscriptionId, cSubP->ip));
  LM_T(LmtNotificationSend, ("%s: Port for HTTPS notification: %d", cSubP->subscriptionId, cSubP->port));
  LM_T(LmtNotificationSend, ("%s: Rest for HTTPS notification: %s", cSubP->subscriptionId, cSubP->rest));

  char  url[512];  // FIXME: DON'T Create the URL over and over - store it in the CachedSubscription
  char* rest = cSubP->rest;

  if (rest[0] == '/')
    rest = &rest[1];

  if (cSubP->port > 0)
    snprintf(url, sizeof(url), "%s://%s:%d/%s", cSubP->protocolString, cSubP->ip, cSubP->port, rest);
  else
    snprintf(url, sizeof(url), "%s://%s/%s", cSubP->protocolString, cSubP->ip, rest);

  if (orionldState.multiP == NULL)
  {
    orionldState.multiP = curl_multi_init();
    if (orionldState.multiP == NULL)
    {
      LM_E(("%s: Internal Error: curl_multi_init failed", cSubP->subscriptionId));
      return -1;
    }
  }

  CURL* curlHandleP = curl_easy_init();
  if (curlHandleP == NULL)
  {
    LM_E(("%s: Internal Error: curl_easy_init failed", cSubP->subscriptionId));
    return -1;
  }

  //
  // URL, Verb, ...
  //
  LM_T(LmtNotificationSend, ("%s: URL: %s", cSubP->subscriptionId, url));
  curl_easy_setopt(curlHandleP, CURLOPT_URL, url);
  curl_easy_setopt(curlHandleP, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(curlHandleP, CURLOPT_TIMEOUT_MS, 5000);                     // Timeout - hard-coded to 5 seconds for now ...
  curl_easy_setopt(curlHandleP, CURLOPT_FOLLOWLOCATION, 1L);                   // Follow redirections

  // SSL options
  curl_easy_setopt(curlHandleP, CURLOPT_SSL_VERIFYPEER, 0L);                   // ignore self-signed certificates for SSL end-points
  curl_easy_setopt(curlHandleP, CURLOPT_SSL_VERIFYHOST, 0L);                   // DO NOT verify the certificate's name against host

  //
  // HTTP Headers -
  //
  struct curl_slist* headers = NULL;
  for (int ix = 1; ix < ioVecLen - 2; ix++)
  {
    // must not be CRLF-terminated - have to remove last 2 chars
    char      header[256];
    uint32_t  headerLen = strlen((char*) ioVec[ix].iov_base);
    char*     headerP   = (headerLen < sizeof(header) - 1)? header : kaAlloc(&orionldState.kalloc, headerLen + 1);

    strncpy(headerP, (char*) ioVec[ix].iov_base, headerLen - 1);
    headerP[ioVec[ix].iov_len - 2] = 0;

    LM_T(LmtNotificationHeaders, ("%s: Notification Request Header: '%s'", cSubP->subscriptionId, headerP));
    headers = curl_slist_append(headers, headerP);
  }
  curl_easy_setopt(curlHandleP, CURLOPT_HTTPHEADER, headers);

  // Must remember the curl easy handle and the slist for later cleanup
  curlSaveForLaterCleanup(curlHandleP, headers);

  //
  // Payload Body
  //
  LM_T(LmtNotificationBody, ("%s: Notification Request Body: %s", cSubP->subscriptionId, ioVec[ioVecLen - 1].iov_base));
  curl_easy_setopt(curlHandleP, CURLOPT_POSTFIELDS, (u_int8_t*) ioVec[ioVecLen - 1].iov_base);

  //
  // Is curl to be debugged (CLI parameter)?
  //
  if (debugCurl == true)
  {
    curl_easy_setopt(curlHandleP, CURLOPT_VERBOSE,       1L);
    curl_easy_setopt(curlHandleP, CURLOPT_DEBUGFUNCTION, curlDebug);
  }

  // Debug Incoming HTTP Headers?
  if (lmTraceIsSet(LmtNotificationHeaders) == true)
  {
    curl_easy_setopt(curlHandleP, CURLOPT_HEADERDATA,     cSubP->subscriptionId);
    curl_easy_setopt(curlHandleP, CURLOPT_HEADERFUNCTION, responseHeaderDebug);   // Callback for received headers
  }

  // Debug Incoming HTTP Body?
  if (lmTraceIsSet(LmtNotificationBody) == true)
    curl_easy_setopt(curlHandleP, CURLOPT_WRITEFUNCTION, notificationResponseBody);  // Callback for reading the response body

  // Add easy handler to the multi handler
  curl_multi_add_handle(orionldState.multiP, curlHandleP);

  *curlHandlePP = curlHandleP;
  return -2;  // All good, just not a file description to await responses for - libcurl does that for us
}
