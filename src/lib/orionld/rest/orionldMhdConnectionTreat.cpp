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
#include <uuid/uuid.h>                                         // uuid_t, uuid_generate_time_safe, uuid_unparse_lower

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBufferCreate.h"                              // kjBufferCreate
#include "kjson/kjParse.h"                                     // kjParse
#include "kjson/kjRender.h"                                    // kjRender
#include "kjson/kjClone.h"                                     // kjClone
#include "kjson/kjFree.h"                                      // kjFree
#include "kjson/kjBuilder.h"                                   // kjString, ...
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kalloc/kaAlloc.h"                                    // kaAlloc
}

#include "common/string.h"                                     // FT
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderAdd, httpHeaderLinkAdd
#include "rest/restReply.h"                                    // restReply

#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/linkCheck.h"                          // linkCheck
#include "orionld/common/SCOMPARE.h"                           // SCOMPARE
#include "orionld/common/CHECK.h"                              // CHECK
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/context/orionldCoreContext.h"                // ORIONLD_DEFAULT_CONTEXT_URL
#include "orionld/context/orionldContextCreateFromUrl.h"       // orionldContextCreateFromUrl
#include "orionld/context/orionldContextAppend.h"              // orionldContextAppend
#include "orionld/context/orionldContextTreat.h"               // orionldContextTreat
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/serviceRoutines/orionldBadVerb.h"            // orionldBadVerb
#include "orionld/rest/orionldServiceInit.h"                   // orionldRestServiceV
#include "orionld/rest/orionldServiceLookup.h"                 // orionldServiceLookup
#include "orionld/rest/temporaryErrorPayloads.h"               // Temporary Error Payloads
#include "orionld/rest/orionldMhdConnectionTreat.h"            // Own Interface



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
// NOTE
//   For this function to work properly, the payload must have been parsed, so that we know whether there is
//   a "@context" member as part of the payload or not.
//
static bool contentTypeCheck(ConnectionInfo* ciP)
{
  if ((ciP->verb != POST) && (ciP->verb != PATCH))
    return true;

  if (orionldState.requestTree == NULL)
    return true;  // No error detected about Content-Type, error postponed to later check

  if (orionldState.requestTree->type != KjObject)  // FIXME: Are all payloads JSON Objects ... ?
    return true;  // No error detected about Content-Type, error postponed to later check


  //
  // Checking that Content-Type is consistent with how context is added
  // - application/ld+json:
  //     @context MUST be in payload
  //     HTTP Link header cannot be present
  // - application/json:
  //     @context cannot be in payload
  //     HTTP Link header may or not be present
  //
  char* errorTitle           = NULL;
  char* errorDetails         = NULL;

  if (orionldState.ngsildContent == true)
  {
    LM_T(LmtContext, ("Content-Type is: application/ld+json"));

    if (orionldState.linkHttpHeaderPresent == true)
    {
      errorTitle   = (char*) "@context in Link HTTP Header";
      errorDetails = (char*) "For application/ld+json, the @context must come inside the JSON payload, NOT in HTTP Header";
    }

    if (orionldState.payloadContextNode == NULL)
    {
      errorTitle   = (char*) "@context missing in JSON payload";
      errorDetails = (char*) "For application/ld+json, the @context must be present in the JSON payload";
    }
  }
  else
  {
    LM_T(LmtContext, ("Content-Type is: application/json"));

    if (orionldState.payloadContextNode != NULL)
    {
      errorTitle   = (char*) "Mismatch between /Content-Type/ and contents of the request payload body";
      errorDetails = (char*) "Content-Type is application/json, yet a '@context' item was present in the payload body";
    }
  }

  if (errorTitle != NULL)
  {
    LM_E(("Bad Input (%s: %s)", errorTitle, errorDetails));

    orionldErrorResponseCreate(OrionldBadRequestData, errorTitle, errorDetails, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;

    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// acceptHeaderExtractAndCheck -
//
static bool acceptHeaderExtractAndCheck(ConnectionInfo* ciP)
{
  bool  explicit_application_json   = false;
  bool  explicit_application_jsonld = false;
  float weight_application_json     = 0;
  float weight_application_jsonld   = 0;

  LM_T(LmtAccept, ("ciP->httpHeaders.accept == %s", ciP->httpHeaders.accept.c_str()));
  LM_T(LmtAccept, ("ciP->httpHeaders.acceptHeaderV.size() == %d", ciP->httpHeaders.acceptHeaderV.size()));

  if (ciP->httpHeaders.acceptHeaderV.size() == 0)
  {
    orionldState.acceptJson   = true;   // Default Accepted MIME-type is application/json
    orionldState.acceptJsonld = false;
    ciP->outMimeType          = JSON;
  }

  // LM_TMP(("LINK: Before: acceptJsonld: %s", FT(orionldState.acceptJsonld)));
  // LM_TMP(("LINK: Before: acceptJson:   %s", FT(orionldState.acceptJson)));

  for (unsigned int ix = 0; ix < ciP->httpHeaders.acceptHeaderV.size(); ix++)
  {
    const char* mediaRange = ciP->httpHeaders.acceptHeaderV[ix]->mediaRange.c_str();

    LM_T(LmtAccept, ("ciP->Accept header %d: '%s'", ix, mediaRange));

    if (SCOMPARE12(mediaRange, 'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/'))
    {
      const char* appType = &mediaRange[12];

      LM_T(LmtAccept, ("mediaRange is application/..."));

      if (SCOMPARE8(appType, 'l', 'd', '+', 'j', 's', 'o', 'n', 0))
      {
        orionldState.acceptJsonld   = true;
        explicit_application_jsonld = true;
        weight_application_jsonld   = ciP->httpHeaders.acceptHeaderV[ix]->qvalue;
      }
      else if (SCOMPARE5(appType, 'j', 's', 'o', 'n', 0))
      {
        orionldState.acceptJson     = true;
        explicit_application_json   = true;
        weight_application_json     = ciP->httpHeaders.acceptHeaderV[ix]->qvalue;
      }
      else if (SCOMPARE2(appType, '*', 0))
      {
        orionldState.acceptJsonld = true;
        orionldState.acceptJson   = true;
      }

      // LM_TMP(("LINK: acceptJsonld: %s", FT(orionldState.acceptJsonld)));
      // LM_TMP(("LINK: acceptJson:   %s", FT(orionldState.acceptJson)));
    }
    else if (SCOMPARE4(mediaRange, '*', '/', '*', 0))
    {
      orionldState.acceptJsonld = true;
      orionldState.acceptJson   = true;
      LM_T(LmtAccept, ("*/* - both json and jsonld OK"));
    }
  }

  // LM_TMP(("LINK: acceptJsonld: %s", FT(orionldState.acceptJsonld)));
  // LM_TMP(("LINK: acceptJson:   %s", FT(orionldState.acceptJson)));

  if ((orionldState.acceptJsonld == false) && (orionldState.acceptJson == false))
  {
    const char* title   = "invalid mime-type";
    const char* details = "HTTP Header /Accept/ contains neither 'application/json' nor 'application/ld+json'";

    LM_W(("Bad Input (HTTP Header /Accept/ contains neither 'application/json' nor 'application/ld+json')"));
    orionldErrorResponseCreate(OrionldBadRequestData, title, details, OrionldDetailsString);
    ciP->httpStatusCode = SccNotAcceptable;

    return false;
  }

  if ((explicit_application_json == true) && (explicit_application_jsonld == false))
    orionldState.acceptJsonld = false;

  if ((weight_application_json != 0) || (weight_application_jsonld != 0))
  {
    if (weight_application_json > weight_application_jsonld)
      orionldState.acceptJsonld = false;
  }

  if (orionldState.acceptJsonld == true)
    ciP->outMimeType = JSONLD;

  return true;
}



// -----------------------------------------------------------------------------
//
// serviceLookup - lookup the Service
//
// orionldMhdConnectionInit guarantees that a valid verb is used. I.e. POST, GET, DELETE or PATCH
// orionldServiceLookup makes sure the URL supprts the verb
//
static OrionLdRestService* serviceLookup(ConnectionInfo* ciP)
{
  OrionLdRestService* serviceP;

  serviceP = orionldServiceLookup(ciP, &orionldRestServiceV[ciP->verb]);
  if (serviceP == NULL)
  {
    if (orionldBadVerb(ciP) == true)
      ciP->httpStatusCode = SccBadVerb;
    else
    {
      orionldErrorResponseCreate(OrionldInvalidRequest, "Service Not Found", orionldState.urlPath, OrionldDetailsString);
      ciP->httpStatusCode = SccContextElementNotFound;
    }
  }

  return serviceP;
}



// -----------------------------------------------------------------------------
//
// payloadEmptyCheck -
//
static bool payloadEmptyCheck(ConnectionInfo* ciP)
{
  // No payload?
  if (ciP->payload == NULL)
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "payload missing", NULL, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  // Empty payload?
  if (ciP->payload[0] == 0)
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "payload missing", NULL, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}


#if 0
// -----------------------------------------------------------------------------
//
// kjTreeFirstLevelPresent
//
static void kjTreeFirstLevelPresent(const char* what, KjNode* tree)
{
  // LM_TMP(("--------------- %s ----------------", what));

  if (tree == NULL)
  {
    // LM_TMP(("Empty tree"));
    return;
  }

  int ix = 0;
  for (KjNode* nodeP = tree->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    // LM_TMP(("node %d: %s", ix, nodeP->name));
    ++ix;
  }
}
#endif



// -----------------------------------------------------------------------------
//
// kjNodeDecouple -
//
static void kjNodeDecouple(KjNode* nodeToDecouple, KjNode* prev, KjNode* parent)
{
  // kjTreeFirstLevelPresent("Before decoupling", parent);

  if (prev != NULL)
    prev->next = nodeToDecouple->next;
  else
    parent->value.firstChildP = nodeToDecouple->next;

  // kjTreeFirstLevelPresent("After decoupling", parent);
}



// -----------------------------------------------------------------------------
//
// uuidGenerate -
//
static void uuidGenerate(char* buf)
{
  uuid_t uuid;

  uuid_generate_time_safe(uuid);
  uuid_unparse_lower(uuid, buf);
}



// -----------------------------------------------------------------------------
//
// payloadParseAndExtractSpecialFields -
//
static bool payloadParseAndExtractSpecialFields(ConnectionInfo* ciP, bool* contextToBeCashedP)
{
  //
  // Parse the payload
  //
  orionldState.requestTree = kjParse(orionldState.kjsonP, ciP->payload);

  //
  // Parse Error?
  //
  if (orionldState.requestTree == NULL)
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "JSON Parse Error", orionldState.kjsonP->errorString, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // Empty payload object?  ("{}" resulting in a tree with one Object that has no children)
  //
  if ((orionldState.requestTree->type == KjObject) && (orionldState.requestTree->value.firstChildP == NULL))
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "Empty Object", "{}", OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // Empty payload array?  ("[]" resulting in a tree with one Object that has no children)
  //
  if ((orionldState.requestTree->type == KjArray) && (orionldState.requestTree->value.firstChildP == NULL))
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "Empty Array", "[]", OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  LM_T(LmtPayloadParse, ("All good - payload parsed. orionldState.requestTree at %p", orionldState.requestTree));

  //
  // Looking up "@context" attribute at first level in payload
  // Checking also for duplicates.
  //
  // If ORIONLD_SERVICE_OPTION_PREFETCH_ENTITY_ID is set in Service Options, also look up entity::id,type
  //
  bool idAndType = ((orionldState.serviceP != NULL) && (orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE));

  if (idAndType)
  {
    KjNode* prev      = NULL;
    KjNode* attrNodeP = orionldState.requestTree->value.firstChildP;

    // kjTreeFirstLevelPresent("Before Removing", orionldState.requestTree);
    while (attrNodeP != NULL)
    {
      if (attrNodeP->name == NULL)
      {
        attrNodeP = attrNodeP->next;
        continue;
      }

      if (SCOMPARE9(attrNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
      {
        if (orionldState.payloadContextNode != NULL)
        {
          LM_W(("Bad Input (duplicated attribute: '@context'"));
          orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", "@context", OrionldDetailsString);
          return false;
        }
        orionldState.payloadContextNode = attrNodeP;
        // LM_TMP(("CTX: Found @context in payload: %p, orionldState.payloadContextNode"));
        LM_T(LmtContext, ("Found @context in the payload (%p)", orionldState.payloadContextNode));

        attrNodeP = orionldState.payloadContextNode->next;
        kjNodeDecouple(orionldState.payloadContextNode, prev, orionldState.requestTree);
      }
      else if (SCOMPARE3(attrNodeP->name, 'i', 'd', 0))
      {
        if (orionldState.payloadIdNode != NULL)
        {
          LM_W(("Bad Input (duplicated attribute: 'Entity:id'"));
          orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", "Entity:id", OrionldDetailsString);
          return false;
        }

        orionldState.payloadIdNode = attrNodeP;
        STRING_CHECK(orionldState.payloadIdNode, "entity id");
        LM_T(LmtContext, ("Found Entity::id in the payload (%p)", orionldState.payloadIdNode));

        attrNodeP = orionldState.payloadIdNode->next;
        kjNodeDecouple(orionldState.payloadIdNode, prev, orionldState.requestTree);
      }
      else if (SCOMPARE5(attrNodeP->name, 't', 'y', 'p', 'e', 0))
      {
        if (orionldState.payloadTypeNode != NULL)
        {
          LM_W(("Bad Input (duplicated attribute: 'Entity:type'"));
          orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", "Entity:type", OrionldDetailsString);
          return false;
        }

        orionldState.payloadTypeNode = attrNodeP;

        STRING_CHECK(orionldState.payloadTypeNode, "entity type");
        extern bool orionldValidName(char* name, char** detailsPP);
        char* details;
        if (orionldValidName(orionldState.payloadTypeNode->value.s, &details) == false)
        {
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid entity type name", details, OrionldDetailsString);
          return false;
        }
        LM_T(LmtContext, ("Found Entity::type in the payload (%p)", orionldState.payloadTypeNode));

        attrNodeP = orionldState.payloadTypeNode->next;
        kjNodeDecouple(orionldState.payloadTypeNode, prev, orionldState.requestTree);
      }
      else
      {
        prev      = attrNodeP;
        attrNodeP = attrNodeP->next;
      }
    }
  }
  else
  {
    KjNode* prev = NULL;

    for (KjNode* attrNodeP = orionldState.requestTree->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
    {
      if (attrNodeP->name == NULL)
        continue;

      if (SCOMPARE9(attrNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
      {
        if (orionldState.payloadContextNode != NULL)
        {
          LM_W(("Bad Input (duplicated attribute: '@context'"));
          orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", "@context", OrionldDetailsString);
          return false;
        }

        orionldState.payloadContextNode = attrNodeP;
        LM_T(LmtContext, ("Found a @context in the payload (%p)", orionldState.payloadContextNode));

        kjNodeDecouple(orionldState.payloadContextNode, prev, orionldState.requestTree);
      }

      prev = attrNodeP;
    }
  }


  if (orionldState.payloadContextNode != NULL)
  {
    // A @context in the payload must be a JSON String, Array, or an Object
    if ((orionldState.payloadContextNode->type != KjString) && (orionldState.payloadContextNode->type != KjArray) && (orionldState.payloadContextNode->type != KjObject))
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON Array nor Object nor a String", "@context", OrionldDetailsString);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  //
  // If Content-Type is application/ld+json (@context is in the payload) and Accept does not include application/ld+json,
  // then the context cannot be returned in the response, only a reference to it.
  // We have to create the context in the "context server" (orionld acts as context server) to be able to return the context in the response.
  //
  // All of this only applies if:
  //   o Content-Type == application/ld+json
  //   o The @context in the payload is not a simple JSON String (a URI)
  //   o The HTTP Accept header does not include application/ld+json => context must be returned as an HTTP header (Link)
  //
  if ((orionldState.payloadContextNode != NULL) && (orionldState.acceptJsonld == false) && (orionldState.payloadContextNode->type != KjString))
  {
    *contextToBeCashedP = true;

    //
    // Create the name of the context to be cached - a uuid
    //
    // FIXME: This needs to be discussed in the Bindings document.
    // FIXME: What about creation of Subscriptions? Should be treated the same. Context name == sub id
    //
    char uuid[37];

    uuidGenerate(uuid);
    snprintf(orionldState.linkBuffer, sizeof(orionldState.linkBuffer), "http://%s:%d/ngsi-ld/contexts/%s", hostname, portNo, uuid);
    orionldState.link = orionldState.linkBuffer;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// linkHeaderCheck -
//
static bool linkHeaderCheck(ConnectionInfo* ciP)
{
  char* details;

  if (orionldState.link[0] != '<')
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "invalid Link HTTP header", "link doesn't start with '<'", OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  ++orionldState.link;  // Step over initial '<'

  if (linkCheck(orionldState.link, &details) == false)
  {
    LM_E(("linkCheck: %s", details));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Link HTTP Header", details, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if ((orionldState.contextP = orionldContextCreateFromUrl(ciP, orionldState.link, OrionldUserContext, &details)) == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Failure to create context from URL", details, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// contextNameLookup -
//
static char* contextNameLookup(char* url)
{
  const char*  needle       = "/ngsi-ld/contexts/";
  char*        needleStart  = strstr(url, needle);

  if (needleStart == NULL)
    return NULL;

  return &needleStart[strlen(needle)];
}



// -----------------------------------------------------------------------------
//
// contextToCache -
//
static bool contextToCache(ConnectionInfo* ciP)
{
  //
  // Creating the context in Context Server
  //

  // The Context tree must be cloned, as it is created inside the thread's kjson
  KjNode* clonedTree = kjClone(orionldState.payloadContextNode);

  if (clonedTree == NULL)
  {
    orionldState.contextP = NULL;  // FIXME: Memleak?
    orionldErrorResponseCreate(OrionldInternalError, "Unable to clone context tree - out of memory?", NULL, OrionldDetailsString);
    ciP->httpStatusCode = SccReceiverInternalError;

    return false;
  }

  OrionldContext* contextP = (OrionldContext*) kaAlloc(&kalloc, sizeof(OrionldContext));

  contextP->url       = kaStrdup(&kalloc, orionldState.contextP->url);
  contextP->tree      = clonedTree;
  contextP->type      = orionldState.contextP->type;
  contextP->ignore    = orionldState.contextP->ignore;
  contextP->temporary = false;

  orionldContextListInsert(contextP, false);

  contextP->name      = contextNameLookup(contextP->url);

  return true;
}



// -----------------------------------------------------------------------------
//
// contextToPayload -
//
static void contextToPayload(void)
{
  // LM_TMP(("CTX: Accepts JSON-LD, Service Routine OK - adding context to payload"));
  // If no contest node exists, create it with the default context

  if (orionldState.payloadContextNode == NULL)
  {
    // LM_TMP(("CTX: payloadContextNode == NULL, using Incoming Link header: '%s'", orionldState.link));
    if (orionldState.link == NULL)
      orionldState.payloadContextNode = kjString(orionldState.kjsonP, "@context", ORIONLD_DEFAULT_CONTEXT_URL);
    else
      orionldState.payloadContextNode = kjString(orionldState.kjsonP, "@context", orionldState.link);
  }

  if (orionldState.payloadContextNode == NULL)
  {
    orionldErrorResponseCreate(OrionldInternalError, "Out of memory", NULL, OrionldDetailsString);
    return;
  }

  //
  // Response tree type:
  //   Object: Add @context node as first member
  //   Array:  Create object for the @context, add it to the new object and add the new object as first member of responseTree
  //
  if (orionldState.responseTree->type == KjObject)
  {
    // LM_TMP(("CTX: Adding orionldState.payloadContextNode to payload"));
    orionldState.payloadContextNode->next = orionldState.responseTree->value.firstChildP;
    orionldState.responseTree->value.firstChildP = orionldState.payloadContextNode;
  }
  else if (orionldState.responseTree->type == KjArray)
  {
    for (KjNode* rTreeItemP = orionldState.responseTree->value.firstChildP; rTreeItemP != NULL; rTreeItemP = rTreeItemP->next)
    {
      KjNode* contextNode;

      if (orionldState.payloadContextNode == NULL)
      {
        if (orionldState.link == NULL)
          contextNode = kjString(orionldState.kjsonP, "@context", ORIONLD_DEFAULT_CONTEXT_URL);
        else
          contextNode = kjString(orionldState.kjsonP, "@context", orionldState.link);
      }
      else
      {
        contextNode = kjClone(orionldState.payloadContextNode);

        orionldStateDelayedKjFree(contextNode);
      }

      if (contextNode == NULL)
      {
        orionldErrorResponseCreate(OrionldInternalError, "Out of memory", NULL, OrionldDetailsString);
        return;
      }

      contextNode->next = rTreeItemP->value.firstChildP;
      rTreeItemP->value.firstChildP = contextNode;
    }
  }
  else
  {
    // Any other type ??? Error
  }
}



// -----------------------------------------------------------------------------
//
// orionldMhdConnectionTreat -
//
// The @context is completely taken care of here in this function.
// Service routines will only use the @context for lookups, everything else is done here, once and for all
//
// What does this function do?
//
//   First of all, this is a callback function, it is called by MHD (libmicrohttpd) when MHD has received an entire
//   request, with HTTP Headers, URI parameters and ALL the payload.
//
//   Actually, that is not entirely true. The callback function for MHD is set to 'connectionTreat', from lib/rest/rest.cpp,
//   and 'connectionTreat' has been programmer to call this function when the entire request has been read.
//
//
//   01. Check for predected error
//   02. Look up the Service
//   03. Check for empty payload for POST/PATCH/PUT
//   04. Parse the payload
//   05. Check for empty payload ( {}, [] )
//   06. Lookup "@context" member, remove it from the request tree - same with "entity::id" and "entity::type" if the request type needs it
//       - orionldState.payloadContextTree    (KjNode*)
//       - orionldState.payloadEntityIdTree   (KjNode*)
//       - orionldState.payloadEntityTypeTree (KjNode*)
//   07. Check for HTTP Link header
//   08. Make sure Context-Type is consistent with HTTP Link Header and Payload Context
//   09. Make sure @context member is valid
//   10. Check the Accept header and decide output MIME-type
//   11. Make sure the HTTP Header "Link" is valid
//   12. Check the @context in HTTP Header
//   13. if (Link):     orionldState.contextP = orionldContextCreateFromUrl()
//   14. if (@context): orionldState.contextP orionldContextCreateFromTree()
//   15. if (@context != SimpleString): Create OrionldContext with 13|14
//   16. if (@context != SimpleString): Insert context in context cache
//   17. Call the SERVICE ROUTINE
//   18. If the service routine failed (returned FALSE), but no HTTP status ERROR code is set, the HTTP status code defaults to 400
//   19. Check for existing responseTree, in case of httpStatusCode >= 400 (except for 405)
//   20. If (orionldState.acceptNgsild): Add orionldState.payloadContextTree to orionldState.responseTree
//   21. If (orionldState.acceptNgsi):   Set HTTP Header "Link" to orionldState.contextP->url
//   22. Render response tree
//   23. IF accept == app/json, add the Link HTTP header
//   24. REPLY
//   25. Cleanup
//   26. DONE
//
//
//
int orionldMhdConnectionTreat(ConnectionInfo* ciP)
{
  bool     contextToBeCashed    = false;
  bool     serviceRoutineResult = false;

  LM_T(LmtMhd, ("Read all the payload - treating the request!"));
  LM_TMP(("----------------------- Treating NGSI-LD request %03d: %s %s: %s --------------------------", requestNo, orionldState.verbString, orionldState.urlPath, ciP->payload));

  //
  // 01. Predetected Error?
  //
  if (ciP->httpStatusCode != SccOk)
    goto respond;

  //
  // 02. Lookup the Service
  //
  // Invalid Verb is checked for in orionldMhdConnectionInit()
  //
  if ((orionldState.serviceP = serviceLookup(ciP)) == NULL)
    goto respond;


  //
  // 03. Check for empty payload for POST/PATCH/PUT
  //
  if (((ciP->verb == POST) || (ciP->verb == PATCH) || (ciP->verb == PUT)) && (payloadEmptyCheck(ciP) == false))
    goto respond;


  //
  // 04. Parse the payload, and check for empty payload, also, find @context in payload and check it's OK
  //
  if ((ciP->payload != NULL) && (payloadParseAndExtractSpecialFields(ciP, &contextToBeCashed) == false))
    goto respond;


  //
  // 05. Check the Content-Type
  //
  if (contentTypeCheck(ciP) == false)
    goto respond;


  //
  // 06. Check the Accept header and ...
  //
  if (acceptHeaderExtractAndCheck(ciP) == false)
    goto respond;


  //
  // 07. Check the @context in HTTP Header, if present
  //
  // NOTE: orionldState.link is set by httpHeaderGet() in rest.cpp, called by orionldMhdConnectionInit()
  //
  if ((orionldState.linkHttpHeaderPresent == true) && (linkHeaderCheck(ciP) == false))
    goto respond;

  // LM_TMP(("orionldState.payloadContextNode at %p", orionldState.payloadContextNode));
  //
  // Treat the context from the payload, if any
  //
  if ((orionldState.payloadContextNode != NULL) && (orionldContextTreat(ciP, orionldState.payloadContextNode) == false))
    goto respond;


  // ********************************************************************************************
  //
  // Call the SERVICE ROUTINE
  //
  LM_T(LmtServiceRoutine, ("Calling Service Routine %s", orionldState.serviceP->url));
  serviceRoutineResult = orionldState.serviceP->serviceRoutine(ciP);
  LM_T(LmtServiceRoutine, ("service routine '%s %s' done", orionldState.verbString, orionldState.serviceP->url));

  //
  // If the service routine failed (returned FALSE), but no HTTP status ERROR code is set,
  // the HTTP status code defaults to 400
  //
  if (serviceRoutineResult == false)
  {
    if (ciP->httpStatusCode < 400)
      ciP->httpStatusCode = SccBadRequest;
  }
  else if ((contextToBeCashed == true) && (contextToCache(ciP) == false))
    goto respond;  // Yes, I know, the label 'respond' comes right after ...


  LM_TMP(("PATCH: After service routine - we still have to send the response"));
 respond:
  //
  // For error responses, there is ALWAYS payload, describing the error
  // If, for some reason (bug!) this payload is missing, then we add a generic error response here
  //
  // The only exception is 405 that has no payload - the info comes in the "Accepted" HTTP header.
  //
  if ((ciP->httpStatusCode >= 400) && (orionldState.responseTree == NULL) && (ciP->httpStatusCode != 405))
    orionldErrorResponseCreate(OrionldInternalError, "Unknown Error", "The reason for this error is unknown", OrionldDetailsString);

  //
  // On error, the Content-Type is always "application/json" and there is NO Link header
  //
  if (ciP->httpStatusCode >= 400)
  {
    orionldState.useLinkHeader = false;
    serviceRoutineResult       = false;  // Just in case ...
    // MimeType handled in restReply()
  }

  //
  // Context in Link HTTP header?
  //    YES - if payloadParseAndExtractSpecialFields() says so (setting contextToBeCashed to true)
  //     NO - if service routine resulted in error
  // OR
  //    YES - if JSONLD is accepted
  //    NO  - if there isn't any payload!
  //    NO  - if the service routine explicitly has asked to not include the Link HTTP header in the response
  //

#if 0
  LM_TMP(("LINK: Add Link to HTTP Header?"));
  LM_TMP(("LINK: contextToBeCashed:          %s", FT(contextToBeCashed)));
  LM_TMP(("LINK: serviceRoutineResult:       %s", FT(serviceRoutineResult)));
  LM_TMP(("LINK: orionldState.acceptJsonld:  %s", FT(orionldState.acceptJsonld)));
  LM_TMP(("LINK: orionldState.responseTree:  %p", orionldState.responseTree));
  LM_TMP(("LINK: orionldState.useLinkHeader: %s", FT(orionldState.useLinkHeader)));
#endif

  if ((contextToBeCashed == true) && (serviceRoutineResult == true))
    httpHeaderLinkAdd(ciP, orionldState.link);
  else if ((orionldState.acceptJsonld == false) && (orionldState.responseTree != NULL) && (orionldState.useLinkHeader == true))
    httpHeaderLinkAdd(ciP, orionldState.link);

  //
  // Is there a KJSON response tree to render?
  //
  if (orionldState.responseTree != NULL)
  {
    //
    // Should a @context be added to the response payload?
    //
    bool addContext = ((orionldState.serviceP != NULL) &&
                       ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD) == 0) &&
                       (orionldState.acceptJsonld == true));

    if (addContext)
    {
      if ((orionldState.acceptJsonld == true) && (ciP->httpStatusCode < 300))
        contextToPayload();
    }


    //
    // Render the payload to get a string for restReply to send the response
    //
    // FIXME: Smarter allocation !!!
    //
    int bufLen = 1024 * 1024 * 32;
    orionldState.responsePayload = (char*) malloc(bufLen);
    if (orionldState.responsePayload != NULL)
    {
      orionldState.responsePayloadAllocated = true;
      kjRender(orionldState.kjsonP, orionldState.responseTree, orionldState.responsePayload, bufLen);
    }
    else
    {
      LM_E(("Error allocating buffer for response payload"));
      orionldErrorResponseCreate(OrionldInternalError, "Out of memory", NULL, OrionldDetailsString);
    }
  }

  if (orionldState.responsePayload != NULL)
    restReply(ciP, orionldState.responsePayload);    // orionldState.responsePayload freed and NULLed by restReply()
  else
    restReply(ciP, "");


  //
  // Cleanup
  //
  orionldStateRelease();

  return MHD_YES;
}
