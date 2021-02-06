/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include <string.h>                                              // strncpy
#include <string>                                                // std::string

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBufferCreate.h"                                // kjBufferCreate
#include "kjson/kjParse.h"                                       // kjParse
#include "kjson/kjRender.h"                                      // kjRender
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjFree.h"                                        // kjFree
#include "kjson/kjBuilder.h"                                     // kjString, ...
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "common/string.h"                                       // FT
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderAdd, httpHeaderLinkAdd
#include "rest/restReply.h"                                      // restReply

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/types/OrionldGeoIndex.h"                       // OrionldGeoIndex
#include "orionld/common/orionldState.h"                         // orionldState, orionldHostName
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/linkCheck.h"                            // linkCheck
#include "orionld/common/SCOMPARE.h"                             // SCOMPARE
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/orionldTenantLookup.h"                  // orionldTenantLookup
#include "orionld/common/orionldTenantCreate.h"                  // orionldTenantCreate
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/performance.h"                          // REQUEST_PERFORMANCE
#include "orionld/db/dbConfiguration.h"                          // dbGeoIndexCreate
#include "orionld/db/dbGeoIndexLookup.h"                         // dbGeoIndexLookup
#include "orionld/payloadCheck/pcheckName.h"                     // pcheckName
#include "orionld/context/orionldCoreContext.h"                  // ORIONLD_CORE_CONTEXT_URL
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/context/orionldContextFromTree.h"              // orionldContextFromTree
#include "orionld/context/orionldContextUrlGenerate.h"           // orionldContextUrlGenerate
#include "orionld/serviceRoutines/orionldPatchAttribute.h"       // orionldPatchAttribute
#include "orionld/rest/OrionLdRestService.h"                     // ORIONLD_URIPARAM_LIMIT, ...
#include "orionld/rest/uriParamName.h"                           // uriParamName
#include "orionld/rest/temporaryErrorPayloads.h"                 // Temporary Error Payloads
#include "orionld/rest/orionldMhdConnectionTreat.h"              // Own Interface



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

    orionldErrorResponseCreate(OrionldBadRequestData, errorTitle, errorDetails);
    orionldState.httpStatusCode = 400;

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

  if (ciP->httpHeaders.acceptHeaderV.size() == 0)
  {
    orionldState.acceptJson   = true;   // Default Accepted MIME-type is application/json
    orionldState.acceptJsonld = false;
    ciP->outMimeType          = JSON;
  }

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
    }
    else if (SCOMPARE4(mediaRange, '*', '/', '*', 0))
    {
      orionldState.acceptJsonld = true;
      orionldState.acceptJson   = true;
      LM_T(LmtAccept, ("*/* - both json and jsonld OK"));
    }
  }

  if ((orionldState.acceptJsonld == false) && (orionldState.acceptJson == false))
  {
    const char* title   = "invalid mime-type";
    const char* details = "HTTP Header /Accept/ contains neither 'application/json' nor 'application/ld+json'";

    LM_W(("Bad Input (HTTP Header /Accept/ contains neither 'application/json' nor 'application/ld+json')"));
    orionldErrorResponseCreate(OrionldBadRequestData, title, details);
    orionldState.httpStatusCode = SccNotAcceptable;

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
// payloadEmptyCheck -
//
static bool payloadEmptyCheck(ConnectionInfo* ciP)
{
  // No payload?
  if (ciP->payload == NULL)
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "payload missing", NULL);
    orionldState.httpStatusCode = 400;
    return false;
  }

  // Empty payload?
  if (ciP->payload[0] == 0)
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "payload missing", NULL);
    orionldState.httpStatusCode = 400;
    return false;
  }

  return true;
}



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
// payloadParseAndExtractSpecialFields -
//
static bool payloadParseAndExtractSpecialFields(ConnectionInfo* ciP, bool* contextToBeCashedP)
{
  //
  // Parse the payload
  //
#ifdef REQUEST_PERFORMANCE
    kTimeGet(&timestamps.parseStart);
#endif
  orionldState.requestTree = kjParse(orionldState.kjsonP, ciP->payload);
#ifdef REQUEST_PERFORMANCE
    kTimeGet(&timestamps.parseEnd);
#endif

  //
  // Parse Error?
  //
  if (orionldState.requestTree == NULL)
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "JSON Parse Error", orionldState.kjsonP->errorString);
    orionldState.httpStatusCode = 400;
    return false;
  }

  //
  // All requests are either arrays or objects
  //
  if ((orionldState.requestTree->type != KjArray) && (orionldState.requestTree->type != KjObject))
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "Invalid Payload", "The payload data must be either a JSON Array or a JSON Object");
    orionldState.httpStatusCode = 400;
    return false;
  }

  //
  // Empty payload object?  ("{}" resulting in a tree with one Object that has no children)
  //
  if ((orionldState.requestTree->type == KjObject) && (orionldState.requestTree->value.firstChildP == NULL))
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "Invalid Payload Body", "Empty Object");
    orionldState.httpStatusCode = 400;
    return false;
  }

  //
  // Empty payload array?  ("[]" resulting in a tree with one Object that has no children)
  //
  if ((orionldState.requestTree->type == KjArray) && (orionldState.requestTree->value.firstChildP == NULL))
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "Invalid Payload Body", "Empty Array");
    orionldState.httpStatusCode = 400;
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
          orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", "@context");
          orionldState.httpStatusCode = 400;
          return false;
        }
        orionldState.payloadContextNode = attrNodeP;
        LM_T(LmtContext, ("Found @context in the payload (%p)", orionldState.payloadContextNode));

        attrNodeP = orionldState.payloadContextNode->next;
        kjNodeDecouple(orionldState.payloadContextNode, prev, orionldState.requestTree);
      }
      else if (SCOMPARE3(attrNodeP->name, 'i', 'd', 0) || SCOMPARE4(attrNodeP->name, '@', 'i', 'd', 0))
      {
        if (orionldState.payloadIdNode != NULL)
        {
          LM_W(("Bad Input (duplicated attribute: 'Entity:id'"));
          orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", "Entity:id");
          orionldState.httpStatusCode = 400;
          return false;
        }

        orionldState.payloadIdNode = attrNodeP;
        STRING_CHECK(orionldState.payloadIdNode, "entity id");
        LM_T(LmtContext, ("Found Entity::id in the payload (%p)", orionldState.payloadIdNode));

        attrNodeP = orionldState.payloadIdNode->next;
        kjNodeDecouple(orionldState.payloadIdNode, prev, orionldState.requestTree);
      }
      else if (SCOMPARE5(attrNodeP->name, 't', 'y', 'p', 'e', 0) || SCOMPARE6(attrNodeP->name, '@', 't', 'y', 'p', 'e', 0))
      {
        if (orionldState.payloadTypeNode != NULL)
        {
          LM_W(("Bad Input (duplicated attribute: 'Entity:type'"));
          orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", "Entity:type");
          orionldState.httpStatusCode = 400;
          return false;
        }

        orionldState.payloadTypeNode = attrNodeP;

        STRING_CHECK(orionldState.payloadTypeNode, "entity type");

        char* detail;
        if (pcheckName(orionldState.payloadTypeNode->value.s, &detail) == false)
        {
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid entity type name", orionldState.payloadTypeNode->value.s);
          orionldState.httpStatusCode = 400;
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
          orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", "@context");
          orionldState.httpStatusCode = 400;
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
      orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON Array nor Object nor a String", "@context");
      orionldState.httpStatusCode = 400;
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldErrorResponseFromProblemDetails
//
void orionldErrorResponseFromProblemDetails(OrionldProblemDetails* pdP)
{
  orionldErrorResponseCreate(pdP->type, pdP->title, pdP->detail);
  orionldState.httpStatusCode = pdP->status;
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
    orionldErrorResponseCreate(OrionldBadRequestData, "invalid Link HTTP header", "link doesn't start with '<'");
    orionldState.httpStatusCode = 400;
    return false;
  }

  ++orionldState.link;  // Step over initial '<'

  if (linkCheck(orionldState.link, &details) == false)
  {
    LM_E(("linkCheck: %s", details));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Link HTTP Header", details);
    orionldState.httpStatusCode = 400;
    return false;
  }

  //
  // The HTTP headers live in the thread. Once the thread dies, the mempry is freed.
  // When calling orionldContextFromUrl, the URL must be properly allocated.
  // As it will be inserted in the Context Cache, that must survive requests, it must be
  // allocated in the global allocation buffer 'kalloc', not the thread-local 'orionldState.kalloc'.
  //
  char*                  url = kaStrdup(&kalloc, orionldState.link);
  OrionldProblemDetails  pd;

  orionldState.contextP = orionldContextFromUrl(url, &pd);
  if (orionldState.contextP == NULL)
  {
    LM_W(("Bad Input? (%s: %s)", pd.title, pd.detail));
    orionldErrorResponseFromProblemDetails(&pd);
    orionldState.httpStatusCode = (HttpStatusCode) pd.status;
    return false;
  }

  orionldState.link = orionldState.contextP->url;

  return true;
}



// -----------------------------------------------------------------------------
//
// contextToPayload -
//
static void contextToPayload(void)
{
  // If no contest node exists, create it with the default context
  if (orionldState.payloadContextNode == NULL)
  {
    if (orionldState.link == NULL)
      orionldState.payloadContextNode = kjString(orionldState.kjsonP, "@context", ORIONLD_CORE_CONTEXT_URL);
    else
      orionldState.payloadContextNode = kjString(orionldState.kjsonP, "@context", orionldState.link);
  }

  if (orionldState.payloadContextNode == NULL)
  {
    LM_E(("Out of memory"));
    orionldErrorResponseCreate(OrionldInternalError, "Out of memory", NULL);
    orionldState.httpStatusCode = 500;  // If ever able to send the response ...
    return;
  }

  //
  // Response tree type:
  //   Object: Add @context node as first member
  //   Array:  Create object for the @context, add it to the new object and add the new object as first member of responseTree
  //
  if (orionldState.responseTree->type == KjObject)
  {
    orionldState.payloadContextNode->next        = orionldState.responseTree->value.firstChildP;
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
          contextNode = kjString(orionldState.kjsonP, "@context", ORIONLD_CORE_CONTEXT_URL);
        else
          contextNode = kjString(orionldState.kjsonP, "@context", orionldState.link);
      }
      else
        contextNode = kjClone(orionldState.kjsonP, orionldState.payloadContextNode);

      if (contextNode == NULL)
      {
        orionldErrorResponseCreate(OrionldInternalError, "Out of memory", NULL);
        orionldState.httpStatusCode = 500;  // If ever able to send the response ...
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
// dbGeoIndexes -
//
static void dbGeoIndexes(void)
{
  char  tenant[64];
  char* tenantP;

  if ((orionldState.tenant == NULL) || (orionldState.tenant[0] == 0))
    tenantP = dbName;
  else
  {
    snprintf(tenant, sizeof(tenant), "%s-%s", dbName, orionldState.tenant);
    tenantP = tenant;
  }

  // sem_take
  for (int ix = 0; ix < orionldState.geoAttrs; ix++)
  {
    if (dbGeoIndexLookup(tenantP, orionldState.geoAttrV[ix]->name) == NULL)
      dbGeoIndexCreate(tenantP, orionldState.geoAttrV[ix]->name);
  }
  // sem_give
}



// -----------------------------------------------------------------------------
//
// uriParamSupport - are all given URI parameters supported by the service?
//
bool uriParamSupport(uint32_t supported, uint32_t given, char** detailP)
{
  int shifts = 0;

  while (given != 0)
  {
    if ((given & 1) != 0)
    {
      if ((supported & (1 << shifts)) == 0)
      {
        *detailP = (char*) uriParamName(1 << shifts);
        return false;
      }
    }

    given = given >> 1;
    ++shifts;
  }

  return true;
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
//   13. if (Link):     orionldState.contextP = orionldContextFromUrl()
//   14. if (@context): orionldState.contextP orionldContextFromTree()
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
static __thread char responsePayload[1024 * 1024];
MHD_Result orionldMhdConnectionTreat(ConnectionInfo* ciP)
{
  bool     contextToBeCashed    = false;
  bool     serviceRoutineResult = false;

  LM_T(LmtMhd, ("Read all the payload - treating the request!"));

  //
  // Predetected Error from orionldMhdConnectionInit?
  //
  if (orionldState.httpStatusCode != 200)
    goto respond;

  //
  // Any URI param given but not supported?
  //
  char* detail;
  if (uriParamSupport(orionldState.serviceP->uriParams, orionldState.uriParams.mask, &detail) == false)
  {
    LM_W(("Bad Input (unsupported URI parameter: %s)", detail));
    orionldErrorResponseCreate(OrionldBadRequestData, "Unsupported URI parameter", detail);
    orionldState.httpStatusCode = 400;
    goto respond;
  }

  //
  // If a tenant is used (HTTP Header NGSILD-Tenant) and it's not any of:
  //   * POST /ngsi-ld/v1/entities
  //   * POST /ngsi-ld/v1/entityOperations/create
  //   * POST /ngsi-ld/v1/entityOperations/upsert
  // then if the tenant doesn't exist, an error must be returned (404)
  //
  if ((orionldState.tenant != NULL) && (*orionldState.tenant != 0))
  {
    if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_MAKE_SURE_TENANT_EXISTS) == ORIONLD_SERVICE_OPTION_MAKE_SURE_TENANT_EXISTS)
    {
      if (orionldTenantLookup(orionldState.tenant) == NULL)
      {
        LM_W(("Bad Input (non-existing tenant: '%s')", orionldState.tenant));
        orionldErrorResponseCreate(OrionldNonExistingTenant, "No such tenant", orionldState.tenant);
        orionldState.httpStatusCode = 404;
        goto respond;
      }
    }
  }


  //
  // 03. Check for empty payload for POST/PATCH/PUT
  //
  if (((ciP->verb == POST) || (ciP->verb == PATCH) || (ciP->verb == PUT)) && (payloadEmptyCheck(ciP) == false))
    goto respond;


  //
  // Save a copy of the incoming payload before it is destroyed during kjParse AND
  // parse the payload, and check for empty payload, also, find @context in payload and check it's OK
  //
  if (ciP->payload != NULL)
  {
    if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_CLONE_PAYLOAD) == 0)
      orionldState.requestPayload = ciP->payload;
    else
      orionldState.requestPayload = kaStrdup(&orionldState.kalloc, ciP->payload);

    if (payloadParseAndExtractSpecialFields(ciP, &contextToBeCashed) == false)
      goto respond;
  }

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

  //
  // Treat inline context
  //
  if (orionldState.payloadContextNode != NULL)
  {
    OrionldProblemDetails pd = { OrionldBadRequestData, (char*) "naught", (char*) "naught", 0 };

    char* id  = NULL;
    char* url = NULL;

    if (orionldState.payloadContextNode->type == KjString)
      url = NULL;  // orionldState.payloadContextNode->value.s
    else
      url = orionldContextUrlGenerate(&id);

    orionldState.contextP = orionldContextFromTree(url, true, orionldState.payloadContextNode, &pd);
    if (orionldState.contextP == NULL)
    {
      LM_W(("Bad Input (invalid inline context. %s: %s)", pd.title, pd.detail));
      orionldErrorResponseFromProblemDetails(&pd);
      orionldState.httpStatusCode = (HttpStatusCode) pd.status;

      goto respond;
    }

    if (id != NULL)
      orionldState.contextP->id = id;

    if (pd.status == 200)  // got an array with only Core Context
      orionldState.contextP = orionldCoreContextP;

    if (pd.status >= 400)
    {
      LM_W(("Bad Input? (%s: %s (type == %d, status = %d))", pd.title, pd.detail, pd.type, pd.status));
      orionldErrorResponseFromProblemDetails(&pd);
      orionldState.httpStatusCode = (HttpStatusCode) pd.status;

      goto respond;
    }
  }

  if (orionldState.contextP == NULL)
    orionldState.contextP = orionldCoreContextP;

  orionldState.link = orionldState.contextP->url;


  // -----------------------------------------------------------------------------
  //
  // Call the SERVICE ROUTINE
  //
#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.serviceRoutineStart);
#endif

  serviceRoutineResult = orionldState.serviceP->serviceRoutine(ciP);

#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.serviceRoutineEnd);
#endif

  //
  // If the service routine failed (returned FALSE), but no HTTP status ERROR code is set,
  // the HTTP status code defaults to 400
  //
  if (serviceRoutineResult == false)
  {
    if (orionldState.httpStatusCode < 400)
      orionldState.httpStatusCode = 400;
  }
  else  // Service Routine worked
  {
    if (orionldState.geoAttrs > 0)
      dbGeoIndexes();

    // New tenant?
    if ((orionldState.tenant != NULL) && (orionldState.tenant[0] != 0))
    {
      if ((orionldState.verb == POST) || (orionldState.verb == PATCH))
      {
        if (orionldTenantLookup(orionldState.tenant) == NULL)
        {
          char prefixed[64];

          snprintf(prefixed, sizeof(prefixed), "%s-%s", dbName, orionldState.tenant);
          orionldTenantCreate(prefixed);

          if (idIndex == true)
            dbIdIndexCreate(prefixed);
        }
      }
    }
  }

 respond:

  //
  // For error responses, there is ALWAYS payload, describing the error
  // If, for some reason (bug!) this payload is missing, then we add a generic error response here
  //
  // The only exception is 405 that has no payload - the info comes in the "Accepted" HTTP header.
  //
  if ((orionldState.httpStatusCode >= 400) && (orionldState.responseTree == NULL) && (orionldState.httpStatusCode != 405))
  {
    orionldErrorResponseCreate(OrionldInternalError, "Unknown Error", "The reason for this error is unknown");
    orionldState.httpStatusCode = 500;
  }

  //
  // On error, the Content-Type is always "application/json" and there is NO Link header
  //
  if (orionldState.httpStatusCode >= 400)
  {
    orionldState.noLinkHeader  = true;   // We don't want the Link header for erroneous requests
    serviceRoutineResult       = false;  // Just in case ...
    // MimeType handled in restReply()
  }

  //
  // Normally, the @context is returned in the HTTP Link header if:
  // * Accept: appplication/json
  // * No Error
  //
  // Need to discuss any exceptions with NEC.
  // E.g.
  //   What if "Accept: appplication/ld+json" in a creation request?
  //   Creation requests have no payload data so the context can't be put in the payload ...
  //
  // What is clear is that no @context is to be returned for error reponses.
  // Also, if there is no payload data in the response, no need for @context
  // Also, GET /.../contexts/{context-id} should NOT give back the link header
  //
  if ((serviceRoutineResult == true) && (orionldState.noLinkHeader == false) && (orionldState.responseTree != NULL))
  {
    if (orionldState.acceptJsonld == false)
      httpHeaderLinkAdd(ciP, orionldState.link);
    else if (orionldState.responseTree == NULL)
      httpHeaderLinkAdd(ciP, orionldState.link);
  }

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
      if ((orionldState.acceptJsonld == true) && (orionldState.httpStatusCode < 300))
        contextToPayload();
    }

    //
    // Render the payload to get a string for restReply to send the response
    //
    // FIXME: Smarter allocation !!!
    //
#ifdef REQUEST_PERFORMANCE
    kTimeGet(&timestamps.renderStart);
#endif
    kjRender(orionldState.kjsonP, orionldState.responseTree, responsePayload, sizeof(responsePayload));

#ifdef REQUEST_PERFORMANCE
    kTimeGet(&timestamps.renderEnd);
#endif
    orionldState.responsePayload = responsePayload;
  }

  //
  // restReply assumes that the HTTP Status Code for the response is in 'ciP->httpStatusCode'
  // FIXME: make the HTTP Status Code a parameter for restReply
  //
  ciP->httpStatusCode = (HttpStatusCode) orionldState.httpStatusCode;

#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.restReplyStart);
#endif

  if (orionldState.responsePayload != NULL)
    restReply(ciP, orionldState.responsePayload);    // orionldState.responsePayload freed and NULLed by restReply()
  else
    restReply(ciP, "");

#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.restReplyEnd);
#endif

  //
  // FIXME: Delay until requestCompleted. The call to orionldStateRelease as well
  //
  // Call TRoE Routine (if there is one) to save the TRoE data.
  // Only if the Service Routine was successful, of course
  //
  if ((orionldState.httpStatusCode >= 200) && (orionldState.httpStatusCode <= 300))
  {
    if ((orionldState.serviceP != NULL) && (orionldState.serviceP->troeRoutine != NULL))
    {
      //
      // Also, if something went wrong during processing, the SR can flag this by setting the requestTree to NULL
      //
      if (orionldState.troeError == true)
        LM_E(("Internal Error (something went wrong during TRoE processing)"));
      else
      {
        numberToDate(orionldState.requestTime, orionldState.requestTimeString, sizeof(orionldState.requestTimeString));

#ifdef REQUEST_PERFORMANCE
        kTimeGet(&timestamps.troeStart);
#endif

        orionldState.serviceP->troeRoutine(ciP);

#ifdef REQUEST_PERFORMANCE
        kTimeGet(&timestamps.troeEnd);
#endif
      }
    }
  }

  //
  // Cleanup
  //
  orionldStateRelease();

#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.requestPartEnd);
#endif

  return MHD_YES;
}
