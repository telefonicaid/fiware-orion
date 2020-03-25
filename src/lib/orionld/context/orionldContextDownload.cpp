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
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails, orionldProblemDetailsFill
#include "orionld/common/orionldState.h"                         // orionldState, contextDownloadAttempts, ...
#include "orionld/common/orionldErrorResponse.h"                 // OrionldBadRequestData, ...
#include "orionld/common/urlParse.h"                             // urlParse
#include "orionld/common/orionldRequestSend.h"                   // orionldRequestSend
#include "orionld/context/orionldContextDownload.h"              // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextDownload -
//
char* orionldContextDownload(const char* url, bool* downloadFailedP, OrionldProblemDetails* pdP)
{
  bool reqOk = false;

  *downloadFailedP = true;

  char                 protocol[16];
  char                 ip[256];
  uint16_t             port    = 0;
  char*                urlPath = NULL;

  if ((url == NULL || *url == 0))
  {
    pdP->type   = OrionldBadRequestData;
    pdP->title  = (char*) "Invalid @context";
    pdP->detail = (char*) ((url == NULL)? "Null @context" : "Empty @context");
    pdP->status = 400;

    return NULL;
  }

  if (urlParse(url, protocol, sizeof(protocol), ip, sizeof(ip), &port, &urlPath, &pdP->detail) == false)
  {
    // pdP->detail set by urlParse
    pdP->type   = OrionldBadRequestData;
    pdP->title  = (char*) "Invalid @context";
    pdP->status = 400;

    return NULL;
  }

  *downloadFailedP = false;

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
    bool              tryAgain = false;
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
                               &pdP->detail,
                               &tryAgain,
                               downloadFailedP,
                               "Accept: application/ld+json",
                               NULL,
                               NULL,
                               0,
                               headerV);

    if (reqOk == true)
      break;

    LM_E(("orionldRequestSend failed (try number %d out of %d. Timeout is: %dms): %s", tries + 1, contextDownloadAttempts, contextDownloadTimeout, pdP->detail));
    if (tryAgain == false)
      break;
  }

  if (reqOk == false)
  {
    pdP->type   = OrionldLdContextNotAvailable;
    pdP->title  = (char*) "Unable to download context";
    pdP->detail = (char*) url;
    pdP->status = 503;

    return NULL;
  }

  return orionldState.httpResponse.buf;
}
