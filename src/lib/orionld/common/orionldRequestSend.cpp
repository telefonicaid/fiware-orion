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
* Author: Ken Zangelin
*/
#include <strings.h>                                           // bcopy
#include <curl/curl.h>                                         // curl

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrlContext, ...
#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/common/urlParse.h"                           // urlParse
#include "orionld/common/orionldRequestSend.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// writeCallback -
//
static size_t writeCallback(void* contents, size_t size, size_t members, void* userP)
{
  size_t                  bytesToCopy  = size * members;
  OrionldResponseBuffer*  rBufP        = (OrionldResponseBuffer*) userP;
  int                     xtraBytes    = 512;

  if (bytesToCopy + rBufP->used >= rBufP->size)
  {
    if (rBufP->buf == rBufP->internalBuffer)
    {
      rBufP->buf  = (char*) malloc(rBufP->size + bytesToCopy + xtraBytes);

      if (rBufP->buf == NULL)
        LM_X(1, ("Runtime Error (out of memory)"));

      rBufP->size = rBufP->size + bytesToCopy + xtraBytes;

      if (rBufP->used > 0)  // Copy contents from internal buffer that got too small
      {
        memcpy(rBufP->buf, rBufP->internalBuffer, rBufP->used);
      }
    }
    else
    {
      rBufP->buf = (char*) realloc(rBufP->buf, rBufP->size + bytesToCopy + xtraBytes);
      rBufP->size = rBufP->size + bytesToCopy + xtraBytes;
    }

    if (rBufP->buf == NULL)
      LM_X(1, ("Runtime Error (out of memory)"));

    //
    // Save pointer to allocated buffer for later call to free()
    //
    orionldState.delayedFreePointer = rBufP->buf;
  }

  memcpy(&rBufP->buf[rBufP->used], contents, bytesToCopy);

  rBufP->used += bytesToCopy;
  rBufP->buf[rBufP->used] = 0;

  return bytesToCopy;
}



// -----------------------------------------------------------------------------
//
// orionldRequestSend - send a request and await its response
//
bool orionldRequestSend
(
  OrionldResponseBuffer*  rBufP,
  const char*             protocol,
  const char*             ip,
  uint16_t                port,
  const char*             urlPath,
  int                     tmoInMilliSeconds,
  char**                  detailPP,
  bool*                   tryAgainP,
  bool*                   downloadFailedP,
  const char*             acceptHeader
)
{
  CURLcode             cCode;
  struct curl_context  cc;
  char                 url[256];

  if (orionldState.delayedFreePointer != NULL)
  {
    orionldStateDelayedFreeEnqueue(orionldState.delayedFreePointer);
    orionldState.delayedFreePointer = NULL;
  }

  *tryAgainP = false;

  if (rBufP->buf == NULL)
  {
    rBufP->size       = 2048;
    rBufP->used       = 0;
    rBufP->allocated  = true;
    rBufP->buf        = (char*) malloc(2048);

    if (rBufP == NULL)
      LM_X(1, ("Out of memory"));

    orionldState.delayedFreePointer = rBufP->buf;  // Saved the pointer to be freed once the request thread ends
  }

  if (port != 0)
  {
    if (urlPath != NULL)
      snprintf(url, sizeof(url), "%s://%s:%d%s", protocol, ip, port, urlPath);
    else
      snprintf(url, sizeof(url), "%s://%s:%d", protocol, ip, port);
  }
  else
  {
    if (urlPath != NULL)
      snprintf(url, sizeof(url), "%s://%s%s", protocol, ip, urlPath);
    else
      snprintf(url, sizeof(url), "%s://%s", protocol, ip);
  }

  LM_T(LmtRequestSend, ("protocol: %s", protocol));
  LM_T(LmtRequestSend, ("IP:       %s", ip));
  LM_T(LmtRequestSend, ("port:     %d", port));
  LM_T(LmtRequestSend, ("URL Path: %s", urlPath));
  LM_T(LmtRequestSend, ("URL:      %s", url));

  get_curl_context(ip, &cc);
  if (cc.curl == NULL)
  {
    LM_E(("Internal Error (Unable to obtain CURL context)"));

    *detailPP        = (char*) "Unable to obtain CURL context";
    rBufP->buf       = NULL;
    *downloadFailedP = true;
    return false;
  }


  //
  // Prepare the CURL handle
  //
  curl_easy_setopt(cc.curl, CURLOPT_URL, url);                             // Set the URL Path
  curl_easy_setopt(cc.curl, CURLOPT_CUSTOMREQUEST, "GET");                 // Set the HTTP verb
  curl_easy_setopt(cc.curl, CURLOPT_FOLLOWLOCATION, 1L);                   // Allow redirection
  curl_easy_setopt(cc.curl, CURLOPT_WRITEFUNCTION, writeCallback);         // Callback function for writes
  curl_easy_setopt(cc.curl, CURLOPT_WRITEDATA, rBufP);                     // Custom data for response handling
  curl_easy_setopt(cc.curl, CURLOPT_TIMEOUT_MS, tmoInMilliSeconds);        // Timeout
  curl_easy_setopt(cc.curl, CURLOPT_FAILONERROR, true);                    // Fail On Error - to detect 404 etc.
  curl_easy_setopt(cc.curl, CURLOPT_FOLLOWLOCATION, 1L);                   // Follow redirections

  struct curl_slist* headers = NULL;

  if (acceptHeader != NULL)
  {
    LM_TMP(("FWD: acceptHeader: '%s'", acceptHeader));
    headers = curl_slist_append(headers, acceptHeader);
    curl_easy_setopt(cc.curl, CURLOPT_HTTPHEADER, headers);
  }

  LM_T(LmtRequestSend, ("Calling curl_easy_perform for GET %s", url));
  cCode = curl_easy_perform(cc.curl);
  LM_T(LmtRequestSend, ("curl_easy_perform returned %d", cCode));

  if (cCode != CURLE_OK)
  {
    LM_E(("Internal Error (curl_easy_perform returned error code %d)", cCode));
    *detailPP  = (char*) url;
    rBufP->buf = NULL;

    if (headers != NULL)
      curl_slist_free_all(headers);

    release_curl_context(&cc);
    LM_E(("curl_easy_perform error %d", cCode));

    *tryAgainP = true;  // FIXME: might depend on cCode ...

    *downloadFailedP = true;
    return false;
  }

  // The downloaded buffer is in rBufP->buf
  LM_T(LmtRequestSend, ("Got response: %s", rBufP->buf));

  release_curl_context(&cc);

  if (headers != NULL)
    curl_slist_free_all(headers);

  *downloadFailedP = false;
  return true;
}
