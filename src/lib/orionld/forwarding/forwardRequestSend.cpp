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
#include <stdio.h>                                               // snprintf
#include <string.h>                                              // strncmp
#include <curl/curl.h>                                           // curl

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjRenderSize.h"                                  // kjFastRenderSize
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/forwarding/forwardRequestSend.h"               // Own interface



// -----------------------------------------------------------------------------
//
// forwardRequestSend -
//
bool forwardRequestSend(ForwardPending* fwdPendingP, const char* dateHeader)
{
  // For now we only support http and https for forwarded requests. libcurl is used for both of them
  // So, a single function with an "if" or two will do - copy from orionld/notifications/httpsNotify.cpp
  //
  // * CURL Handles:  create handle (also multi handle if it does not exist)
  // * URL:           fwdPendingP->url
  // * BODY:          kjFastRender(fwdPendingP->body) + CURLOPT_POSTFIELDS (POST? hmmm ...)
  // * HTTP Headers:  Std headers + loop over fwdPendingP->regP["csourceInfo"], use curl_slist_append
  // * CURL Options:  set a number oftions dfor the handle
  // * SEND:          Actually, just add the "easy handler" to the "multi handler" of orionldState.curlFwdMultiP
  //                  Once all requests are added - send by invoking 'curl_multi_perform(orionldState.curlFwdMultiP, ...)'
  //

  //
  // CURL Handles
  //
  if (orionldState.curlFwdMultiP == NULL)
  {
    orionldState.curlFwdMultiP = curl_multi_init();
    if (orionldState.curlFwdMultiP == NULL)
    {
      LM_E(("Internal Error: curl_multi_init failed"));
      return false;
    }
  }

  fwdPendingP->curlHandle = curl_easy_init();
  if (fwdPendingP->curlHandle == NULL)
  {
    LM_E(("Internal Error: curl_easy_init failed"));
    return false;
  }


  //
  // URL
  //
  KjNode* endpointP = kjLookup(fwdPendingP->regP->regTree, "endpoint");
  char    url[128];

  snprintf(url, sizeof(url), "%s/ngsi-ld/v1/entities", endpointP->value.s);
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_URL, url);
  LM(("FWD: URL: '%s'", url));

  bool https = (strncmp(endpointP->value.s, "https", 5) == 0);


  //
  // BODY
  //
  int   approxContentLen = kjFastRenderSize(fwdPendingP->body);
  char* payloadBody      = kaAlloc(&orionldState.kalloc, approxContentLen);

  kjFastRender(fwdPendingP->body, payloadBody);
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_POSTFIELDS, payloadBody);


  //
  // HTTP Headers
  // - Content-Length
  // - Content-Type
  // - User-Agent
  // - Date
  // - Those in csourceInfo (if Content-Type is present, it is ignored (for now))
  //
  KjNode*             csourceInfoP = kjLookup(fwdPendingP->regP->regTree, "contextSourceInfo");
  struct curl_slist*  headers     = NULL;

  // Date
  headers = curl_slist_append(headers, dateHeader);

  //
  // Host
  //
  headers = curl_slist_append(headers, hostHeader);

  //
  // Content-Length
  //
  int  contentLen = strlen(payloadBody);
  char contentLenHeader[64];
  snprintf(contentLenHeader, sizeof(contentLenHeader), "Content-Length: %d", contentLen);
  headers = curl_slist_append(headers, contentLenHeader);

  //
  // Content-Type (for now we maintain the original Content-Type in the forwarded request
  // First we remove it from the "headers" curl_slist, so that we can give it ourselves
  //
  const char* contentType = (orionldState.in.contentType == JSON)? "Content-Type: application/json" : "Content-Type: application/ld+json";
  headers = curl_slist_append(headers, contentType);

  //
  // Accept - application/json
  //
  const char* accept = "Accept: application/json";
  headers = curl_slist_append(headers, accept);


  //
  // Custom headers from Registration::contextSourceInfo
  //
  if (csourceInfoP != NULL)
  {
    for (KjNode* regHeaderP = csourceInfoP->value.firstChildP; regHeaderP != NULL; regHeaderP = regHeaderP->next)
    {
      KjNode* keyP   = kjLookup(regHeaderP, "key");
      KjNode* valueP = kjLookup(regHeaderP, "value");

      if ((keyP == NULL) || (valueP == NULL))
      {
        LM_W(("Missing key or value in csourceInfo"));
        continue;
      }

      if ((keyP->type != KjString) || (valueP->type != KjString))
      {
        LM_W(("key or value in csourceInfo is non-string"));
        continue;
      }

      if (strcasecmp(keyP->value.s, "Content-Type") == 0)
      {
        LM_W(("Content-Type is part of the csourceInfo headers - that is not implemented yet, sorry"));
        continue;
      }

      char  header[256];  // Assuming 256 is enough
      snprintf(header, sizeof(header), "%s: %s", keyP->value.s, valueP->value.s);
      headers = curl_slist_append(headers, header);
    }
  }

  // User-Agent
  headers = curl_slist_append(headers, userAgentHeaderNoLF);  // userAgentHeader is initialized in orionldServiceInit()
  // headers = curl_slist_append(headers, "User-Agent: orionld/xxx");  // This works


#if 0
  struct curl_slist* sP = headers;
  while (sP != NULL)
  {
    LM(("FWD: Added header '%s'", sP->data));
    sP = sP->next;
  }
#endif

  //
  // Need to save the pointer to the curl headers in order to free it afterwards
  //
  fwdPendingP->curlHeaders = headers;

  //
  // CURL Options
  //
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_CUSTOMREQUEST, orionldState.verbString);
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_TIMEOUT_MS, 5000);                     // Timeout - hard-coded to 5 seconds for now ...
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_FAILONERROR, true);                    // Fail On Error - to detect 404 etc.
  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_FOLLOWLOCATION, 1L);                   // Follow redirections

  if (https)
  {
    // SSL options
    curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);                 // ignore self-signed certificates for SSL end-points
    curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);                 // DO NOT verify the certificate's name against host
  }

  curl_easy_setopt(fwdPendingP->curlHandle, CURLOPT_HTTPHEADER, headers);

  //
  // SEND (sort of)
  //
  curl_multi_add_handle(orionldState.curlFwdMultiP, fwdPendingP->curlHandle);
  return 0;
}
