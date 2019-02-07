/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <strings.h>                                      // bcopy
#include <curl/curl.h>                                    // curl

#include "logMsg/logMsg.h"                                // LM_*
#include "logMsg/traceLevels.h"                           // Lmt*

#include "orionld/common/urlParse.h"                      // urlParse
#include "orionld/common/orionldRequestSend.h"            // Own interface


#if 0
// -----------------------------------------------------------------------------
//
// memcpyKz -
//
static void memcpyKz(char* to, char* from, int size)
{
  while (size > 0)
  {
    *to = *from;
    --size;
    ++to;
    ++from;
  }
}
#endif


// -----------------------------------------------------------------------------
//
// writeCallback -
//
static size_t writeCallback(void* contents, size_t size, size_t members, void* userP)
{
  size_t                  realSize   = size * members;
  OrionldResponseBuffer*  rBufP      = (OrionldResponseBuffer*) userP;
  int                     xtraBytes  = 512;

  // LM_T(LmtWriteCallback, ("Got a chunk of %d bytes: %s", realSize, (char*) contents));

  char tmpBuf[128];
  strncpy(tmpBuf, rBufP->buf, 128);
  tmpBuf[127] = 0;
  LM_TMP(("Buffer so far (only the first 127 bytes): %s", tmpBuf));

  // LM_TMP(("In writeCallback: Got a chunk of %d bytes", realSize));
  if (realSize + rBufP->used >= rBufP->size)
  {
    if (rBufP->buf == rBufP->internalBuffer)
    {
      // LM_TMP(("In writeCallback: need to allocate (size: %d)", rBufP->size + realSize + xtraBytes));
      rBufP->buf  = (char*) malloc(rBufP->size + realSize + xtraBytes);

      if ((rBufP->buf != NULL) && (rBufP->used > 0))  // Copy contents from internal buffer that got too small
      {
        memcpy(rBufP->buf, rBufP->internalBuffer, rBufP->used);
        rBufP->size = rBufP->size + realSize + xtraBytes;
      }
    }
    else
    {
      // LM_TMP(("In writeCallback: need to reallocate (new size: %d)", rBufP->size + realSize + xtraBytes));
      rBufP->buf = (char*) realloc(rBufP->buf, rBufP->size + realSize + xtraBytes);
      rBufP->size = rBufP->size + realSize + xtraBytes;
    }

    if (rBufP->buf == NULL)
    {
      LM_E(("Runtime Error (out of memory)"));
      return 0;
    }
  }

  LM_TMP(("In writeCallback: copying to position %d-%d in rBufP->buf (%d bytes to be copied). Buf size: %d", rBufP->used, rBufP->used + realSize, realSize, rBufP->size));
  memcpy(&rBufP->buf[rBufP->used], contents, realSize);
  LM_TMP(("Copy worked!"));

  rBufP->used += realSize;
  rBufP->buf[rBufP->used] = 0;

  return realSize;
}



// -----------------------------------------------------------------------------
//
// orionldRequestSend - send a request and await its response
//
bool orionldRequestSend
(
  OrionldResponseBuffer*  rBufP,
  const char*             url,
  int                     tmoInMilliSeconds,
  char**                  detailsPP,
  bool*                   tryAgainP
)
{
  CURLcode             cCode;
  struct curl_context  cc;
  char                 protocol[16];
  char                 ip[256];
  uint16_t             port    = 0;
  char*                urlPath = NULL;

  *tryAgainP = false;

  if (urlParse(url, protocol, sizeof(protocol), ip, sizeof(ip), &port, &urlPath, detailsPP) == false)
  {
    // urlParse sets *detailsPP

    // This function must release the allocated respose buffer in case of errpr
    if (rBufP->buf != rBufP->internalBuffer)
      free(rBufP->buf);
    rBufP->buf = NULL;

    LM_E(("urlParse failed for url '%s': %s", url, *detailsPP));
    return false;
  }

  LM_T(LmtRequestSend, ("protocol: %s", protocol));
  LM_T(LmtRequestSend, ("IP:       %s", ip));
  LM_T(LmtRequestSend, ("URL Path: %s", urlPath));

  get_curl_context(ip, &cc);
  if (cc.curl == NULL)
  {
    *detailsPP = (char*) "Unable to obtain CURL context";

    // This function must release the allocated respose buffer in case of error
    if (rBufP->buf != rBufP->internalBuffer)
      free(rBufP->buf);
    rBufP->buf = NULL;

    LM_E((*detailsPP));
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

#if 0
  curl_easy_setopt(cc.curl, CURLOPT_HEADER, 1);                            // Include header in body output
  curl_easy_setopt(cc.curl, CURLOPT_HTTPHEADER, headers);                  // Put headers in place
#endif

  LM_T(LmtRequestSend, ("Calling curl_easy_perform for GET %s", url));
  // LM_TMP(("Calling curl_easy_perform for GET %s", url));
  cCode = curl_easy_perform(cc.curl);
  LM_T(LmtRequestSend, ("curl_easy_perform returned %d", cCode));
  // LM_TMP(("curl_easy_perform returned %d", cCode));
  if (cCode != CURLE_OK)
  {
    *detailsPP = (char*) url;

    // This function must release the allocated respose buffer in case of error
    if (rBufP->buf != rBufP->internalBuffer)
      free(rBufP->buf);

    rBufP->buf = NULL;

    release_curl_context(&cc);
    LM_E(("curl_easy_perform error %d", cCode));

    *tryAgainP = true;  // FIXME: might depend on cCode ...

    return false;
  }

  // The downloaded buffer is in rBufP->buf
  LM_T(LmtRequestSend, ("Got response: %s", rBufP->buf));
  // LM_TMP(("Got response: %s", rBufP->buf));

  release_curl_context(&cc);

  return true;
}
