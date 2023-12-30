/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <unistd.h>                                              // NULL

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldResponseErrorType.h"              // OrionldResponseErrorType
#include "orionld/types/OrionldHttpHeader.h"
#include "orionld/common/orionldState.h"                         // orionldState, contextDownloadAttempts, ...
#include "orionld/common/urlParse.h"                             // urlParse
#include "orionld/common/orionldRequestSend.h"                   // orionldRequestSend
#include "orionld/context/orionldContextDownload.h"              // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextDownload -
//
char* orionldContextDownload(const char* url)
{
  char        protocol[16];
  char        ip[256];
  uint16_t    port    = 0;
  char*       urlPath = NULL;
  bool        reqOk   = false;

  if ((url == NULL || *url == 0))
  {
    orionldState.pd.type   = OrionldBadRequestData;
    orionldState.pd.title  = (char*) "Invalid @context";
    orionldState.pd.detail = (char*) ((url == NULL)? "Null @context" : "Empty @context");
    orionldState.pd.status = 400;

    return NULL;
  }

  if (urlParse(url, protocol, sizeof(protocol), ip, sizeof(ip), &port, &urlPath, &orionldState.pd.detail) == false)
  {
    // orionldState.pd.detail set by urlParse
    orionldState.pd.type   = OrionldBadRequestData;
    orionldState.pd.title  = (char*) "Invalid @context";
    orionldState.pd.status = 400;

    return NULL;
  }

  //
  // Downloading the context
  //
  for (int tries = 0; tries < contextDownloadAttempts; tries++)
  {
    orionldState.httpResponse.buf       = NULL;  // orionldRequestSend allocates
    orionldState.httpResponse.size      = 0;
    orionldState.httpResponse.used      = 0;
    orionldState.httpResponse.allocated = false;

    //
    // detailsPP is filled in by orionldRequestSend()
    // orionldState.httpResponse.buf freed by orionldRequestSend() in case of error
    //
    bool              tryAgain       = false;
    bool              downloadFailed = false;
    OrionldHttpHeader headerV[1];

    headerV[0].type = HttpHeaderNone;

    reqOk = orionldRequestSend(&orionldState.httpResponse,
                               protocol,
                               ip,
                               port,
                               "GET",
                               urlPath,
                               contextDownloadTimeout,
                               NULL,
                               &orionldState.pd.detail,
                               &tryAgain,
                               &downloadFailed,
                               "Accept: application/ld+json",
                               NULL,
                               NULL,
                               0,
                               headerV);

    if (reqOk == true)
      break;

    LM_E(("orionldRequestSend failed (try number %d out of %d. Timeout is: %dms): %s", tries + 1, contextDownloadAttempts, contextDownloadTimeout, orionldState.pd.detail));
    if (tryAgain == false)
      break;
  }

  if (reqOk == false)  // && (downloadFailed == true)? - could get better error handling with 'downloadFailed'
  {
    orionldState.pd.type   = OrionldLdContextNotAvailable;
    orionldState.pd.title  = (char*) "Unable to download context";
    orionldState.pd.detail = (char*) url;
    orionldState.pd.status = 503;

    return NULL;
  }

  return orionldState.httpResponse.buf;
}
