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
extern "C"
{
#include "kjson/kjBufferCreate.h"                     // kjBufferCreate
#include "kjson/kjParse.h"                            // kjParse
#include "kjson/kjRender.h"                           // kjRender
#include "kjson/kjFree.h"                             // kjFree
}

#include "rest/ConnectionInfo.h"                      // ConnectionInfo
#include "rest/restReply.h"                           // restReply

#include "orionld/serviceRoutines/orionldBadVerb.h"   // orionldBadVerb
#include "orionld/rest/orionldServiceInit.h"          // orionldRestServiceV 
#include "orionld/rest/orionldServiceLookup.h"        // orionldServiceLookup
#include "orionld/rest/temporaryErrorPayloads.h"      // Temporary Error Payloads
#include "orionld/rest/orionldMhdConnectionTreat.h"   // Own Interface



// -----------------------------------------------------------------------------
//
// orionldMhdConnectionTreat -
//
int orionldMhdConnectionTreat(ConnectionInfo* ciP)
{
  LM_TMP(("Read all the payload - treating the request!"));

  // If no error predetected, lookup the service and call its service routine
  if (ciP->httpStatusCode == SccOk)
  {
    if ((ciP->verb == POST) || (ciP->verb == GET) || (ciP->verb == DELETE) || (ciP->verb == PATCH))
      ciP->serviceP = orionldServiceLookup(ciP, &orionldRestServiceV[ciP->verb]);

    
    if (ciP->payload != NULL)
    {
      LM_TMP(("parsing the payload '%s'", ciP->payload));

      // FIXME P6: Do we really need to allocate a kjsonP for every request?
      ciP->kjsonP = kjBufferCreate();      
      if (ciP->kjsonP == NULL)
        LM_X(1, ("Out of memory"));

      ciP->kjsonP->spacesPerIndent   = 0;
      ciP->kjsonP->nlString          = (char*) "";
      ciP->kjsonP->stringBeforeColon = (char*) "";
      ciP->kjsonP->stringAfterColon  = (char*) "";
      
      ciP->requestTopP = kjParse(ciP->kjsonP, ciP->payload);
      LM_TMP(("After kjParse"));
      if (ciP->requestTopP == NULL)
        LM_X(1, ("JSON parse error"));
      LM_TMP(("All good - payload parsed"));
    }

    if (ciP->serviceP != NULL)
    {
      LM_TMP(("Calling the service routine"));
      bool b = ciP->serviceP->serviceRoutine(ciP);
      LM_TMP(("service routine done"));

      if (b == false)
      {
        //
        // If the service routine failed (returned FALSE), but no HTTP status code is set,
        // The HTTP status code defaults to 400
        //
        if (ciP->httpStatusCode == SccOk)
        {
          ciP->httpStatusCode = SccBadRequest;
        }
      }
    }
    else
    {
      orionldBadVerb(ciP);
    }
  }


  // Is there a KJSON response tree to render?
  if (ciP->responseTopP != NULL)
  {
    LM_TMP(("Rendering KJSON response tree"));
    ciP->responsePayload          = (char*) malloc(1024);
    ciP->responsePayloadAllocated = true;
    kjRender(ciP->kjsonP, ciP->responseTopP, ciP->responsePayload, 1024);
  }

  if (ciP->responsePayload != NULL)
  {
    LM_TMP(("Responding with '%s'", ciP->responsePayload));
    ciP->outMimeType = JSON;
    restReply(ciP, ciP->responsePayload);
  }
  else
  {
    LM_TMP(("Responding without payload"));
    restReply(ciP, "");
  }

#if 0  // FIXME P9: Fix the leaks!!!
  if (ciP->requestTopP != NULL)
  {
    LM_TMP(("Calling kjFree on request"));
    kjFree(ciP->requestTopP);
    LM_TMP(("kjFree'd request"));
  }
  
  if (ciP->responseTopP != NULL)
  {
    LM_TMP(("Calling kjFree on response"));
    kjFree(ciP->responseTopP);
    LM_TMP(("kjFree'd response"));
  }
#endif

  free(ciP->kjsonP);
  ciP->kjsonP = NULL;

  return MHD_YES;
}
