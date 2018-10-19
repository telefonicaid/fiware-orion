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
#include "orionld/common/SCOMPARE.h"                        // SCOMPARE
#include "orionld/serviceRoutines/orionldBadVerb.h"         // orionldBadVerb
#include "orionld/rest/orionldServiceInit.h"                // orionldRestServiceV
#include "orionld/rest/orionldServiceLookup.h"              // orionldServiceLookup
#include "orionld/context/orionldContextCreateFromUrl.h"    // orionldContextCreateFromUrl
#include "orionld/rest/temporaryErrorPayloads.h"            // Temporary Error Payloads
#include "orionld/rest/orionldMhdConnectionTreat.h"         // Own Interface



// -----------------------------------------------------------------------------
//
// contentTypeCheck -
//
// - Content-Type: application/json + no context at all - OK
// - Content-Type: application/json + context in payload - see error
// - Content-Type: application/json + context in HTTP Header - OK
// - Content-Type: application/ld+json + no context at all - see error
// - Content-Type: application/ld+json + context in payload - OK
// - Content-Type: application/ld+json + context in HTTP Header - see error
// - Content-Type: application/ld+json + context in HTTP Header + context in payload - see error
//
static bool contentTypeCheck(ConnectionInfo* ciP, char** errorTitleP, char** detailsP)
{
  if ((ciP->verb != POST) && (ciP->verb != PATCH))
    return true;

  if (ciP->requestTree == NULL)
  {
    return true;  // No error detected about Content-Type, error postponed to later check
  }

  if (ciP->requestTree->children == NULL)
  {
    return true;  // No error detected about Content-Type, error postponed to later check
  }

  if (ciP->requestTree->type != KjObject)  // FIXME: Are all payloads JSON Objects ... ?
  {
    return true;  // No error detected about Content-Type, error postponed to later check
  }

  // Lookup "@context" attribute
  KjNode* contextNodeP     = ciP->requestTree->children;
  bool    contextInPayload = false;

  while ((contextNodeP != NULL) && (contextInPayload == false))
  {
    if (strcmp(contextNodeP->name, "@context") == 0)
      contextInPayload = true;
    else
      contextNodeP = contextNodeP->next;
  }

  LM_TMP(("Context In JSON Payload: %s", (contextInPayload == true)?    "YES" : "NO"));
  LM_TMP(("Context In HTTP Header:  %s", (ciP->httpHeaders.link != "")? "YES" : "NO"));

  if (strcmp(ciP->httpHeaders.contentType.c_str(), "application/json") == 0)
  {
    LM_TMP(("Content-Type is: application/json"));

    if (contextNodeP != NULL)
    {
      *errorTitleP = (char*) "Invalid MIME-type for @context in payload";
      *detailsP    = (char*) "For @context in payload, the MIME type must be application/ld+json";
      return false;
    }
  }
  else if (strcmp(ciP->httpHeaders.contentType.c_str(), "application/ld+json") == 0)
  {
    LM_TMP(("Content-Type is: application/ld+json"));

    if (ciP->httpHeaders.link != "")
    {
      *errorTitleP = (char*) "@context in Link HTTP Header";
      *detailsP    = (char*) "For application/ld+json, the @context must come inside the JSON payload, NOT in HTTP Header";
      return false;
    }

    if (contextInPayload == false)
    {
      *errorTitleP = (char*) "@context missing in JSON payload";
      *detailsP    = (char*) "For application/ld+json, the @context must be present in the JSON payload";
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// acceptHeaderCheck -
//
static bool acceptHeaderCheck(ConnectionInfo* ciP, char** errorTitleP, char** detailsP)
{
  LM_TMP(("ciP->httpHeaders.accept == %s", ciP->httpHeaders.accept.c_str()));

  for (unsigned int ix = 0; ix < ciP->httpHeaders.acceptHeaderV.size(); ix++)
  {
    const char* mediaRange = ciP->httpHeaders.acceptHeaderV[ix]->mediaRange.c_str();

    LM_TMP(("ciP->Accept header %d: %s", ix, mediaRange));
    if (SCOMPARE12(mediaRange, 'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/'))
    {
      const char* appType = &mediaRange[12];

      if      (SCOMPARE8(appType, 'l', 'd', '+', 'j', 's', 'o', 'n', 0))  ciP->httpHeaders.acceptJsonld = true;
      else if (SCOMPARE5(appType, 'j', 's', 'o', 'n', 0))                 ciP->httpHeaders.acceptJson   = true;
      else if (SCOMPARE2(appType, '*', 0))
      {
        ciP->httpHeaders.acceptJsonld = true;
        ciP->httpHeaders.acceptJson   = true;
      }
    }
    else if (SCOMPARE4(mediaRange, '*', '/', '*', 0) == 0)
    {
      ciP->httpHeaders.acceptJsonld = true;
      ciP->httpHeaders.acceptJson   = true;
    }
  }

  if ((ciP->httpHeaders.acceptJsonld == false) && (ciP->httpHeaders.acceptJson == false))
  {
    *errorTitleP = (char*) "invalid mime-type";
    *detailsP    = (char*) "HTTP Header /Accept/ contains neither 'application/json' nor 'application/ld+json'";

    return false;
  }

  LM_TMP(("Done"));
  return true;
}



// -----------------------------------------------------------------------------
//
// orionldMhdConnectionTreat -
//
int orionldMhdConnectionTreat(ConnectionInfo* ciP)
{
  char* errorTitle;
  char* details;
  bool  error = false;

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
        orionldErrorResponseCreate(ciP, OrionldInvalidRequest, "JSON Parse Error", ciP->kjsonP->errorString, OrionldDetailsString);
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
      if (urlCheck((char*) ciP->httpHeaders.link.c_str(), &details) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Link HTTP Header must be a valid URL", details, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        return false;  // FIXME: Can I return here? What about rendering resuly and calling restReply (end of this function) ?
      }

      if ((ciP->contextP = orionldContextCreateFromUrl(ciP, ciP->httpHeaders.link.c_str(), OrionldUserContext, &details)) == NULL)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Failure to create context from URL", details, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        return false;  // FIXME: Can I return here?
      }
    }
    else
    {
      ciP->contextP = NULL;
    }


    //
    // ContentType Check
    //
    if (error == false)
    {
      if (contentTypeCheck(ciP, &errorTitle, &details) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, errorTitle, details, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        error = true;
      }
    }

    if (error == false)
    {
      if (acceptHeaderCheck(ciP, &errorTitle, &details) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, errorTitle, details, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        error = true;
      }
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
        // the HTTP status code defaults to 400
        //
        if (ciP->httpStatusCode == SccOk)
        {
          ciP->httpStatusCode = SccBadRequest;
        }
      }
    }
  }


  // Is there a KJSON response tree to render?
  // [ Note that this is always TRUE when error == true ]
  //
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
