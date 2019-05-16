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
#include "orionld/common/linkCheck.h"                       // linkCheck
#include "orionld/common/SCOMPARE.h"                        // SCOMPARE
#include "orionld/common/OrionldConnection.h"               // orionldState
#include "orionld/context/orionldContextCreateFromUrl.h"    // orionldContextCreateFromUrl
#include "orionld/serviceRoutines/orionldBadVerb.h"         // orionldBadVerb
#include "orionld/rest/orionldServiceInit.h"                // orionldRestServiceV
#include "orionld/rest/orionldServiceLookup.h"              // orionldServiceLookup
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
static bool contentTypeCheck(ConnectionInfo* ciP, KjNode* contextNodeP, char** errorTitleP, char** detailsP)
{
  if ((ciP->verb != POST) && (ciP->verb != PATCH))
    return true;

  if (ciP->requestTree == NULL)
  {
    return true;  // No error detected about Content-Type, error postponed to later check
  }

  if (ciP->requestTree->value.firstChildP == NULL)
  {
    return true;  // No error detected about Content-Type, error postponed to later check
  }

  if (ciP->requestTree->type != KjObject)  // FIXME: Are all payloads JSON Objects ... ?
  {
    return true;  // No error detected about Content-Type, error postponed to later check
  }


  bool  contextInPayload     = (contextNodeP != NULL);
  bool  contextInHttpHeader  = (ciP->httpHeaders.link != "");

  LM_T(LmtContext, ("Context In JSON Payload: %s", (contextInPayload == true)?    "YES" : "NO"));
  LM_T(LmtContext, ("Context In HTTP Header:  %s", (contextInHttpHeader == true)? "YES" : "NO"));

  if (strcmp(ciP->httpHeaders.contentType.c_str(), "application/json") == 0)
  {
    LM_T(LmtContext, ("Content-Type is: application/json"));

    if (contextInPayload == true)
    {
      *errorTitleP = (char*) "Invalid MIME-type for @context in payload";
      *detailsP    = (char*) "For @context in payload, the MIME type must be application/ld+json";
      return false;
    }
  }
  else if (ciP->httpHeaders.ngsildContent == true)
  {
    LM_T(LmtContext, ("Content-Type is: application/ld+json"));

    if (contextInHttpHeader == true)
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
  LM_T(LmtAccept, ("ciP->httpHeaders.accept == %s", ciP->httpHeaders.accept.c_str()));
  LM_T(LmtAccept, ("ciP->httpHeaders.acceptHeaederV.size() == %d", ciP->httpHeaders.acceptHeaderV.size()));

  if (ciP->httpHeaders.acceptHeaderV.size() == 0)
  {
    ciP->httpHeaders.acceptJson   = true;
    ciP->httpHeaders.acceptJsonld = false;
    ciP->outMimeType              = JSON;
  }

  for (unsigned int ix = 0; ix < ciP->httpHeaders.acceptHeaderV.size(); ix++)
  {
    const char* mediaRange = ciP->httpHeaders.acceptHeaderV[ix]->mediaRange.c_str();

    LM_T(LmtAccept, ("ciP->Accept header %d: '%s'", ix, mediaRange));
    if (SCOMPARE12(mediaRange, 'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/'))
    {
      const char* appType = &mediaRange[12];

      if      (SCOMPARE8(appType, 'l', 'd', '+', 'j', 's', 'o', 'n', 0))  ciP->httpHeaders.acceptJsonld = true;
      else if (SCOMPARE5(appType, 'j', 's', 'o', 'n', 0))                 ciP->httpHeaders.acceptJson   = true;
      else if (SCOMPARE2(appType, '*', 0))
      {
        ciP->httpHeaders.acceptJsonld = true;
        ciP->httpHeaders.acceptJson   = true;
        LM_T(LmtAccept, ("application/* - both json and jsonld OK"));
      }
    }
    else if (SCOMPARE4(mediaRange, '*', '/', '*', 0))
    {
      ciP->httpHeaders.acceptJsonld = true;
      ciP->httpHeaders.acceptJson   = true;
      LM_T(LmtAccept, ("*/* - both json and jsonld OK"));
    }
  }

  if ((ciP->httpHeaders.acceptJsonld == false) && (ciP->httpHeaders.acceptJson == false))
  {
    *errorTitleP = (char*) "invalid mime-type";
    *detailsP    = (char*) "HTTP Header /Accept/ contains neither 'application/json' nor 'application/ld+json'";

    return false;
  }

  if (ciP->httpHeaders.acceptJsonld == true)
    ciP->outMimeType = JSONLD;

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

  LM_T(LmtMhd, ("Read all the payload - treating the request!"));
  LM_TMP(("----------------------- Treating NGSI-LD request %03d: %s %s: %s --------------------------", requestNo, ciP->verbString, ciP->urlPath, ciP->payload));

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
      if (orionldBadVerb(ciP) == true)
      {
        ciP->httpStatusCode = SccBadVerb;
      }
      else
      {
        orionldErrorResponseCreate(ciP, OrionldInvalidRequest, "Service Not Found", ciP->urlPath, OrionldDetailsString);
        ciP->httpStatusCode = SccContextElementNotFound;
      }
      goto respond;
    }

    bool     jsonldLinkInHttpHeader = (ciP->httpHeaders.link != "");  // The URL is extracted later
    KjNode*  contextNodeP           = NULL;

    //
    // Empty/No payload for POST or PATCH (or PUT)
    //
    if ((ciP->verb == POST) || (ciP->verb == PATCH) || (ciP->verb == PUT))
    {
      // No payload
      if (ciP->payload == NULL)
      {
        orionldErrorResponseCreate(ciP, OrionldInvalidRequest, "payload missing", NULL, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        goto respond;
      }

      // Empty payload
      if (ciP->payload[0] == 0)
      {
        orionldErrorResponseCreate(ciP, OrionldInvalidRequest, "payload missing", NULL, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        goto respond;
      }
    }

    //
    // Parsing payload
    //
    if (ciP->payload != NULL)
    {
      ciP->requestTree = kjParse(ciP->kjsonP, ciP->payload);
      LM_T(LmtPayloadParse, ("After kjParse: %p", ciP->requestTree));
      if (ciP->requestTree == NULL)
      {
        orionldErrorResponseCreate(ciP, OrionldInvalidRequest, "JSON Parse Error", ciP->kjsonP->errorString, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        goto respond;
      }

      //
      // Empty payload object?  ("{}" resulting in a tree with one Object that has no children)
      //
      if ((ciP->requestTree->type == KjObject) && (ciP->requestTree->value.firstChildP == NULL))
      {
        orionldErrorResponseCreate(ciP, OrionldInvalidRequest, "Empty Object", "{}", OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        goto respond;
      }

      //
      // Empty payload array?  ("[]" resulting in a tree with one Object that has no children)
      //
      if ((ciP->requestTree->type == KjArray) && (ciP->requestTree->value.firstChildP == NULL))
      {
        orionldErrorResponseCreate(ciP, OrionldInvalidRequest, "Empty Array", "[]", OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        goto respond;
      }

      LM_T(LmtPayloadParse, ("All good - payload parsed. ciP->requestTree at %p", ciP->requestTree));

      if (ciP->requestTree->value.firstChildP != NULL)
        LM_T(LmtPayloadParse, ("Right after kjParse, first child of request is: '%s'", ciP->requestTree->value.firstChildP->name));

      //
      // Looking up "@context" attribute at first level in payload
      //
      for (KjNode* attrNodeP = ciP->requestTree->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
      {
        if (attrNodeP->name == NULL)
          continue;

        if (SCOMPARE9(attrNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
        {
          contextNodeP = attrNodeP;
          LM_T(LmtContext, ("Found a @context in the payload (%p)", contextNodeP));
          break;
        }
      }
    }

    if (contextNodeP == NULL)
      LM_T(LmtContext, ("No @context in payload"));

    //
    // ContentType Check
    //
    if (contentTypeCheck(ciP, contextNodeP, &errorTitle, &details) == false)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, errorTitle, details, OrionldDetailsString);
      ciP->httpStatusCode = SccBadRequest;
      goto respond;
    }

    if (acceptHeaderCheck(ciP, &errorTitle, &details) == false)
    {
      LM_E(("acceptHeaderCheck failed: %s", details));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, errorTitle, details, OrionldDetailsString);
      ciP->httpStatusCode = SccBadRequest;
      goto respond;
    }


    //
    // Checking the @context in HTTP Header
    //
    if (jsonldLinkInHttpHeader == true)
    {
      if (linkCheck((char*) ciP->httpHeaders.link.c_str(), &ciP->httpHeaders.linkUrl, &details) == false)
      {
        LM_E(("linkCheck: %s", details));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Link HTTP Header", details, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        goto respond;
      }

      if ((orionldState.contextP = orionldContextCreateFromUrl(ciP, ciP->httpHeaders.linkUrl, OrionldUserContext, &details)) == NULL)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Failure to create context from URL", details, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        goto respond;
      }
    }
    else
      orionldState.contextP = NULL;

    ciP->contextP = orionldState.contextP;  // FIXME: Stop using ciP->contextP

    //
    // FIXME: Checking the @context from payload ... move from orionldPostEntities()
    //
    LM_T(LmtServiceRoutine, ("Calling Service Routine %s", ciP->serviceP->url));
    bool b = ciP->serviceP->serviceRoutine(ciP);
    LM_T(LmtServiceRoutine,("service routine '%s' done", ciP->serviceP->url));

    if ((ciP->responseTree != NULL) && (ciP->responseTree->value.firstChildP != NULL))
      LM_T(LmtPayloadParse, ("Right after serviceRoutine, first child of response is: '%s'", ciP->responseTree->value.firstChildP->name));

    if (b == false)
    {
      //
      // If the service routine failed (returned FALSE), but no HTTP status ERROR code is set,
      // the HTTP status code defaults to 400
      //
      if (ciP->httpStatusCode < 400)
        ciP->httpStatusCode = SccBadRequest;
    }
  }

respond:
  //
  // For error responses, there is ALWAYS payload, describing the error
  // If, for some reason (bug!) this payload is missing, then we add a generic error response here
  //
  if ((ciP->httpStatusCode >= 400) && (ciP->responseTree == NULL) && (ciP->httpStatusCode != 405))
    orionldErrorResponseCreate(ciP, OrionldInternalError, "Unknown Error", "The reason for this error is unknown", OrionldDetailsString);

  //
  // Is there a KJSON response tree to render?
  //
  if (ciP->responseTree != NULL)
  {
    LM_T(LmtJsonResponse, ("Rendering KJSON response tree"));
    // FIXME: Smarter allocation !!!
    ciP->responsePayload = (char*) malloc(20480);
    if (ciP->responsePayload != NULL)
    {
      ciP->responsePayloadAllocated = true;
      kjRender(ciP->kjsonP, ciP->responseTree, ciP->responsePayload, 20480);
    }
    else
    {
      LM_E(("Error allocating buffer for response payload"));
      //
      // To have any response at all, use a pre-fabricated string:
      //   ciP->responsePayload = ErrorPayloadForAllocationErrors;
      //
      // And in restReply() make sure that (ciP->responsePayload != ErrorPayloadForAllocationErrors)
      // before calling 'free()'
      //
    }
  }

  if (ciP->responsePayload != NULL)
  {
    restReply(ciP, ciP->responsePayload);
    // ciP->responsePayload freed and NULLed by restReply()
  }
  else
  {
    restReply(ciP, "");
  }

  return MHD_YES;
}
