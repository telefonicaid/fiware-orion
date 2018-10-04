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
#include "logMsg/logMsg.h"                                  // LM_*
#include "logMsg/traceLevels.h"                             // Lmt*

extern "C"
{
#include "kjson/kjBufferCreate.h"                           // kjBufferCreate
#include "kjson/kjParse.h"                                  // kjParse
#include "kjson/kjRender.h"                                 // kjRender
#include "kjson/kjFree.h"                                   // kjFree
}

#include "rest/ConnectionInfo.h"                            // ConnectionInfo
#include "rest/restReply.h"                                 // restReply

#include "orionld/common/orionldErrorResponse.h"            // orionldErrorResponseCreate
#include "orionld/common/urlCheck.h"                        // urlCheck
#include "orionld/serviceRoutines/orionldBadVerb.h"         // orionldBadVerb
#include "orionld/rest/orionldServiceInit.h"                // orionldRestServiceV 
#include "orionld/rest/orionldServiceLookup.h"              // orionldServiceLookup
#include "orionld/context/orionldContextCreateFromUrl.h"    // orionldContextCreateFromUrl
#include "orionld/rest/temporaryErrorPayloads.h"            // Temporary Error Payloads
#include "orionld/rest/orionldMhdConnectionTreat.h"         // Own Interface



// -----------------------------------------------------------------------------
//
// orionldMhdConnectionTreat -
//
int orionldMhdConnectionTreat(ConnectionInfo* ciP)
{
  bool error = false;

  LM_T(LmtMhd, ("Read all the payload - treating the request!"));

  // If no error predetected, lookup the service and call its service routine
  if (ciP->httpStatusCode == SccOk)
  {
    //
    // Lookup the Service
    //
    // orionldMhdConnectionInit guarantees that a valid verb is used. I.e. POST, GET, DELETE or PATCH
    // orionldServiceLookup makes sure the URL supprts the verb
    //
    ciP->serviceP = orionldServiceLookup(ciP, &orionldRestServiceV[ciP->verb]);

    if (ciP->serviceP == NULL)
    {
      // FIXME: Can this happen?  BadVerb service routine?
      orionldBadVerb(ciP);
    }

    if (ciP->payload != NULL)
    {
      LM_T(LmtPayloadParse, ("parsing the payload '%s'", ciP->payload));
      
      ciP->requestTree = kjParse(ciP->kjsonP, ciP->payload);
      LM_T(LmtPayloadParse, ("After kjParse: %p", ciP->requestTree));
      if (ciP->requestTree == NULL)
      {
        LM_TMP(("Creating Error Response for JSON Parse Error (%s)", ciP->kjsonP->errorString));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "JSON Parse Error", ciP->kjsonP->errorString, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        error = true;
      }
      LM_T(LmtPayloadParse, ("All good - payload parsed"));
    }

    //
    // Checking for @context in HTTP Header
    //
    if (ciP->httpHeaders.link != "")
    {
      char* details;

      if (urlCheck((char*) ciP->httpHeaders.link.c_str(), &details) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Link HTTP Header must be a valid URL", details, OrionldDetailsString);
        return false;
      }

      if ((ciP->contextP = orionldContextCreateFromUrl(ciP, ciP->httpHeaders.link.c_str(), &details)) == NULL)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from URL", details, OrionldDetailsString);
        return false;
      }
    }
    else
    {
      ciP->contextP = NULL;
    }
    

    if (error == false)
    {
      LM_T(LmtServiceRoutine, ("Calling Service Routine %s", ciP->serviceP->url));
      bool b = ciP->serviceP->serviceRoutine(ciP);
      LM_T(LmtServiceRoutine,("service routine '%s' done", ciP->serviceP->url));
      
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
  }


  // Is there a KJSON response tree to render?
  if (ciP->responseTree != NULL)
  {
    LM_T(LmtJsonResponse, ("Rendering KJSON response tree"));
    ciP->responsePayload = (char*) malloc(1024);
    if (ciP->responsePayload != NULL)
    {
      ciP->responsePayloadAllocated = true;
      kjRender(ciP->kjsonP, ciP->responseTree, ciP->responsePayload, 1024);
    }
    else
    {
      LM_E(("Error allocating buffer for response payload"));
    }
  }
  
  if (ciP->responsePayload != NULL)
  {
    LM_T(LmtJsonResponse, ("Responding with '%s'", ciP->responsePayload));
    ciP->outMimeType = JSON;
    restReply(ciP, ciP->responsePayload);
    // ciP->responsePayload freed and NULLed by restReply()
  }
  else
  {
    LM_T(LmtJsonResponse, ("Responding without payload"));
    restReply(ciP, "");
  }

  LM_TMP(("End of service routine: ciP->contextP at %p", ciP->contextP));
  return MHD_YES;
}
