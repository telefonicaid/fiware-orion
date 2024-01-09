/*
*
* Copyright 2024 FIWARE Foundation e.V.
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
#include <microhttpd.h>                                        // MHD

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjRender.h"                                      // kjRender, kjFastRender
#include "kjson/kjRenderSize.h"                                  // kjRenderSize, kjFastRenderSize
}

#include "logMsg/logMsg.h"

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/performance.h"                        // PERFORMANCE
#include "orionld/mhd/mhdReply.h"                              // Own interface



// -----------------------------------------------------------------------------
//
// mhdReply -
//
void mhdReply(KjNode* body)
{
  unsigned int responsePayloadSize = 0;

  if (body != NULL)
  {
    PERFORMANCE(renderStart);
    responsePayloadSize = (orionldState.uriParams.prettyPrint == false)? kjFastRenderSize(body) : kjRenderSize(orionldState.kjsonP, body);

    if (responsePayloadSize < orionldState.kalloc.bytesLeft + 8)
      orionldState.responsePayload = kaAlloc(&orionldState.kalloc, responsePayloadSize);
    else
    {
      orionldState.responsePayload = (char*) malloc(responsePayloadSize);

      if (orionldState.responsePayload == NULL)
        LM_X(1, ("Out of memory"));

      orionldStateDelayedFreeEnqueue(orionldState.responsePayload);
    }

    if (orionldState.uriParams.prettyPrint == false)
      kjFastRender(body, orionldState.responsePayload);
    else
      kjRender(orionldState.kjsonP, body, orionldState.responsePayload, responsePayloadSize);

    PERFORMANCE(renderEnd);
  }

  PERFORMANCE(mhdReplyStart);

  LM_T(LmtResponse, ("Response Body: '%s'", (body != NULL)? orionldState.responsePayload : "None"));
  LM_T(LmtResponse, ("Response Code:  %d", orionldState.httpStatusCode));

  //
  // Enqueue response
  //
  int           responsePayloadLen = (body != NULL)? strlen(orionldState.responsePayload) : 0;
  MHD_Response* response           = MHD_create_response_from_buffer(responsePayloadLen, orionldState.responsePayload, MHD_RESPMEM_MUST_COPY);

  if (!response)
  {
    LM_E(("Runtime Error (MHD_create_response_from_buffer FAILED)"));
    if (orionldState.responsePayloadAllocated == true)
    {
      free(orionldState.responsePayload);
      orionldState.responsePayload = NULL;
    }

    return;
  }

  //
  // Get the HTTP headers from orionldState.out.headers
  //
  for (int ix = 0; ix < orionldState.out.headers.ix; ix++)
  {
    OrionldHeader* hP = &orionldState.out.headers.headerV[ix];
    MHD_add_response_header(response, orionldHeaderName[hP->type], hP->sValue);
  }

  if (responsePayloadLen > 0)
  {
    char* contentType = (char*) "application/json";

    if      (responsePayloadLen <= 2)                    contentType = (char*) "application/json";
    else if (orionldState.httpStatusCode  >= 400)        contentType = (char*) "application/json";
    else if (orionldState.out.contentType == MT_JSONLD)  contentType = (char*) "application/ld+json";
    else if (orionldState.out.contentType == MT_GEOJSON) contentType = (char*) "application/geo+json";

    MHD_add_response_header(response, "Content-Type", contentType);
  }

  MHD_queue_response(orionldState.mhdConnection, orionldState.httpStatusCode, response);
  MHD_destroy_response(response);
  PERFORMANCE(mhdReplyEnd);
}
