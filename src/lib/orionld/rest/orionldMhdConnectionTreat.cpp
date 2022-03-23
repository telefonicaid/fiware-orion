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
#include <semaphore.h>                                           // sem_wait, sem_post

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
#include "kbase/kStringSplit.h"                                  // kStringSplit
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBufferCreate.h"                                // kjBufferCreate
#include "kjson/kjParse.h"                                       // kjParse
#include "kjson/kjRenderSize.h"                                  // kjRenderSize, kjFastRenderSize
#include "kjson/kjRender.h"                                      // kjRender, kjFastRender
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjFree.h"                                        // kjFree
#include "kjson/kjBuilder.h"                                     // kjString, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderLinkAdd
#include "rest/restReply.h"                                      // restReply

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/types/OrionldGeoIndex.h"                       // OrionldGeoIndex
#include "orionld/common/orionldState.h"                         // orionldState, orionldHostName, coreContextUrl
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/linkCheck.h"                            // linkCheck
#include "orionld/common/SCOMPARE.h"                             // SCOMPARE
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/orionldTenantCreate.h"                  // orionldTenantCreate
#include "orionld/common/orionldTenantLookup.h"                  // orionldTenantLookup
#include "orionld/common/orionldTenantGet.h"                     // orionldTenantGet
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/performance.h"                          // PERFORMANCE
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/db/dbConfiguration.h"                          // dbGeoIndexCreate
#include "orionld/db/dbGeoIndexLookup.h"                         // dbGeoIndexLookup
#include "orionld/kjTree/kjGeojsonEntityTransform.h"             // kjGeojsonEntityTransform
#include "orionld/kjTree/kjGeojsonEntitiesTransform.h"           // kjGeojsonEntitiesTransform
#include "orionld/payloadCheck/pcheckName.h"                     // pcheckName
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/context/orionldContextFromTree.h"              // orionldContextFromTree
#include "orionld/context/orionldContextUrlGenerate.h"           // orionldContextUrlGenerate
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/serviceRoutines/orionldPatchAttribute.h"       // orionldPatchAttribute
#include "orionld/serviceRoutines/orionldGetEntity.h"            // orionldGetEntity
#include "orionld/serviceRoutines/orionldGetEntities.h"          // orionldGetEntities
#include "orionld/serviceRoutines/orionldPostEntities.h"         // orionldPostEntities
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
static bool contentTypeCheck(void)
{
  if ((orionldState.verb != POST) && (orionldState.verb != PATCH))
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

  if (orionldState.in.contentType == JSONLD)
  {
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
// payloadEmptyCheck -
//
static bool payloadEmptyCheck(void)
{
  // No payload?
  if (orionldState.in.payload == NULL)
  {
    orionldErrorResponseCreate(OrionldInvalidRequest, "payload missing", NULL);
    orionldState.httpStatusCode = 400;
    return false;
  }

  // Empty payload?
  if (orionldState.in.payload[0] == 0)
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
static bool payloadParseAndExtractSpecialFields(bool* contextToBeCashedP)
{
  //
  // Parse the payload
  //
  PERFORMANCE(parseStart);
  orionldState.requestTree = kjParse(orionldState.kjsonP, orionldState.in.payload);
  PERFORMANCE(parseEnd);

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

        STRING_CHECK(attrNodeP, "entity id");
        URI_CHECK(attrNodeP->value.s, "id", true);

        orionldState.payloadIdNode = attrNodeP;
        attrNodeP                  = attrNodeP->next;
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

        STRING_CHECK(attrNodeP, "entity type");
        URI_CHECK(attrNodeP->value.s, "type", false);

        orionldState.payloadTypeNode = attrNodeP;
        attrNodeP                    = attrNodeP->next;
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

    if (orionldState.requestTree->type == KjObject)
    {
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

          kjNodeDecouple(orionldState.payloadContextNode, prev, orionldState.requestTree);
        }

        prev = attrNodeP;
      }
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
static bool linkHeaderCheck(void)
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

  orionldState.contextP = orionldContextFromUrl(url, NULL, &pd);
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
      orionldState.payloadContextNode = kjString(orionldState.kjsonP, "@context", coreContextUrl);
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
    KjNode* contextNode = kjLookup(orionldState.responseTree, "@context");

    if (contextNode == NULL)  // Not present - must add it
    {
      orionldState.payloadContextNode->next        = orionldState.responseTree->value.firstChildP;
      orionldState.responseTree->value.firstChildP = orionldState.payloadContextNode;
    }
  }
  else if (orionldState.responseTree->type == KjArray)
  {
    for (KjNode* rTreeItemP = orionldState.responseTree->value.firstChildP; rTreeItemP != NULL; rTreeItemP = rTreeItemP->next)
    {
      KjNode* contextNode = kjLookup(rTreeItemP, "@context");

      if (contextNode != NULL)  // @context already present in payload
        continue;

      if (orionldState.payloadContextNode == NULL)
      {
        if (orionldState.link == NULL)
          contextNode = kjString(orionldState.kjsonP, "@context", coreContextUrl);
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
  // sem_take
  for (int ix = 0; ix < orionldState.geoAttrs; ix++)
  {
    if (dbGeoIndexLookup(orionldState.tenantP->tenant, orionldState.geoAttrV[ix]->name) == NULL)
      dbGeoIndexCreate(orionldState.tenantP, orionldState.geoAttrV[ix]->name);
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


static int commaCount(char* s)
{
  int commas = 0;

  while (*s != 0)
  {
    if (*s == ',')
      ++commas;
    ++s;
  }

  return commas;
}



// -----------------------------------------------------------------------------
//
// pCheckUriParamAttrs -
//
static bool pCheckUriParamAttrs(void)
{
  if (orionldState.uriParams.attrs == NULL)
    return true;

  int   items     = commaCount(orionldState.uriParams.attrs) + 1;
  char* arraysDup = kaStrdup(&orionldState.kalloc, orionldState.uriParams.attrs);  // Keep original value of 'attrs'

  orionldState.in.attrsList.items = items;
  orionldState.in.attrsList.array = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * items);

  if (orionldState.in.attrsList.array == NULL)
  {
    LM_E(("Out of memory (allocating an /attrs/ array of %d char pointers)", items));
    orionldError(OrionldInternalError, "Out of memory", "allocating the array for /attrs/ URI param", 500);
    return false;
  }

  int splitItems = kStringSplit(arraysDup, ',', orionldState.in.attrsList.array, items);

  if (splitItems != items)
  {
    LM_E(("kStringSplit didn't find exactly %d items (it found %d)", items, splitItems));
    orionldError(OrionldInternalError, "Internal Error", "kStringSplit does not agree with commaCount", 500);
    return false;
  }

  for (int item = 0; item < items; item++)
  {
    orionldState.in.attrsList.array[item] = orionldAttributeExpand(orionldState.contextP, orionldState.in.attrsList.array[item], true, NULL);  // Expand-function
  }

  return true;
}




// -----------------------------------------------------------------------------
//
// pCheckUriParamId -
//
// NOTE
//   No need to check ORIONLD_SERVICE_OPTION_EXPAND_TYPE here as the URI-parameter 'type'
//   is only alloowed by those services that need expansion (GET /entities and GET /registrations)
//
static bool pCheckUriParamId(void)
{
  if (orionldState.uriParams.id == NULL)
    return true;

  if (orionldState.uriParams.id[0] == 0)
  {
    orionldError(OrionldBadRequestData, "Invalid Entity ID", "Empty String", 400);
    return false;
  }

  int   items     = commaCount(orionldState.uriParams.id) + 1;
  char* arraysDup = kaStrdup(&orionldState.kalloc, orionldState.uriParams.id);  // Keep original value of 'id'

  orionldState.in.idList.items = items;
  orionldState.in.idList.array = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * items);

  if (orionldState.in.idList.array == NULL)
  {
    LM_E(("Out of memory (allocating an /id/ array of %d char pointers)", items));
    orionldError(OrionldInternalError, "Out of memory", "allocating the array for /id/ URI param", 500);
    return false;
  }

  int splitItems = kStringSplit(arraysDup, ',', orionldState.in.idList.array, items);

  if (splitItems != items)
  {
    LM_E(("kStringSplit didn't find exactly %d items (it found %d)", items, splitItems));
    orionldError(OrionldInternalError, "Internal Error", "kStringSplit does not agree with commaCount", 500);
    return false;
  }

  for (int item = 0; item < items; item++)
  {
    if (pCheckUri(orionldState.in.idList.array[item], "Entity ID in URI param", true) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckUriParamType -
//
// NOTE
//   No need to check ORIONLD_SERVICE_OPTION_EXPAND_TYPE here as the URI-parameter 'type'
//   is only allowed by those services that need expansion (GET /entities and GET /registrations)
//
static bool pCheckUriParamType(void)
{
  if (orionldState.uriParams.type == NULL)
    return true;

  int   items     = commaCount(orionldState.uriParams.type) + 1;
  char* arraysDup = kaStrdup(&orionldState.kalloc, orionldState.uriParams.type);  // Keep original value of 'type'

  orionldState.in.typeList.items = items;
  orionldState.in.typeList.array = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * items);

  if (orionldState.in.typeList.array == NULL)
  {
    LM_E(("Out of memory (allocating an /type/ array of %d char pointers)", items));
    orionldError(OrionldInternalError, "Out of memory", "allocating the array for /type/ URI param", 500);
    return false;
  }

  int splitItems = kStringSplit(arraysDup, ',', orionldState.in.typeList.array, items);

  if (splitItems != items)
  {
    LM_E(("kStringSplit didn't find exactly %d items (it found %d)", items, splitItems));
    orionldError(OrionldInternalError, "Internal Error", "kStringSplit does not agree with commaCount", 500);
    return false;
  }

  for (int item = 0; item < items; item++)
  {
    orionldState.in.typeList.array[item] = orionldContextItemExpand(orionldState.contextP, orionldState.in.typeList.array[item], true, NULL);  // Expand-function
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckUriParamGeoProperty -
//
static bool pCheckUriParamGeoProperty()
{
  if (orionldState.uriParams.geoproperty == NULL)
    return true;

  if ((strcmp(orionldState.uriParams.geoproperty, "location")         != 0) &&
      (strcmp(orionldState.uriParams.geoproperty, "observationSpace") != 0) &&
      (strcmp(orionldState.uriParams.geoproperty, "operationSpace")   != 0))
  {
    orionldState.uriParams.geoproperty = orionldAttributeExpand(orionldState.contextP, orionldState.uriParams.geoproperty, true, NULL);  // Expand-function
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckUriParamGeometryProperty -
//
static bool pCheckUriParamGeometryProperty()
{
  if (orionldState.uriParams.geometryProperty == NULL)
  {
    orionldState.uriParams.geometryProperty  = (char*) "location";
    orionldState.in.geometryPropertyExpanded = (char*) "location";
  }
  else
  {
    if ((strcmp(orionldState.uriParams.geometryProperty, "location")         != 0) &&
        (strcmp(orionldState.uriParams.geometryProperty, "observationSpace") != 0) &&
        (strcmp(orionldState.uriParams.geometryProperty, "operationSpace")   != 0))
    {
      orionldState.in.geometryPropertyExpanded = orionldAttributeExpand(orionldState.contextP, orionldState.uriParams.geometryProperty, true, NULL);
    }
    else
      orionldState.in.geometryPropertyExpanded = orionldState.uriParams.geometryProperty;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckPayloadEntityType -
//
static bool pCheckPayloadEntityType(void)
{
  if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_EXPAND_TYPE) == ORIONLD_SERVICE_OPTION_EXPAND_TYPE)
  {
    if (orionldState.payloadTypeNode != NULL)
      orionldState.payloadTypeNode->value.s = orionldContextItemExpand(orionldState.contextP, orionldState.payloadTypeNode->value.s, true, NULL);  // Expand-function
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckUrlPathAttributeName -
//
static bool pCheckUrlPathAttributeName(void)
{
  if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_EXPAND_ATTR) == ORIONLD_SERVICE_OPTION_EXPAND_ATTR)
  {
    orionldState.in.pathAttrExpanded = orionldAttributeExpand(orionldState.contextP, orionldState.wildcard[1], true, NULL);  // Expand-function
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// uriParamExpansion -
//
// Expand attribute names and entity types.
// Convert comma separated lists (attrs + type + id + ...) into arrays.
//
// Doing the "type" node from the payload body as well. Not a URI param, but close enough
//
static bool uriParamExpansion(void)
{
  if (pCheckUriParamId()               == false) return false;
  if (pCheckUriParamType()             == false) return false;
  if (pCheckUriParamAttrs()            == false) return false;
  if (pCheckUriParamGeoProperty()      == false) return false;
  if (pCheckUriParamGeometryProperty() == false) return false;
  if (pCheckPayloadEntityType()        == false) return false;
  if (pCheckUrlPathAttributeName()     == false) return false;

  // Can't do anything about 'q' - needs to be parsed first - expansion done in 'orionld/common/qParse.cpp'

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
//   20. If (Accept: JSONLD): Add orionldState.payloadContextTree to orionldState.responseTree
//   21. If (Accept: JSON):   Set HTTP Header "Link" to orionldState.contextP->url
//   22. Render response tree
//   23. IF accept == app/json, add the Link HTTP header
//   24. REPLY
//   25. Cleanup
//   26. DONE
//
MHD_Result orionldMhdConnectionTreat(void)
{
  bool     contextToBeCashed    = false;
  bool     serviceRoutineResult = false;

  // If OPTIONS verb, we skip all checks, go straight to the service routine
  if (orionldState.verb == OPTIONS)
    goto serviceRoutine;

  //
  // Predetected Error from orionldMhdConnectionInit?
  //
  if (orionldState.httpStatusCode != 200)
    goto respond;

  // if ((orionldState.in.contentLength > 0) && (orionldState.verb != POST) && (orionldState.verb != PATCH) && (orionldState.verb != PUT) && (orionldState.verb != OPTIONS))
  if ((orionldState.in.contentLength > 0) && (orionldState.verb != POST) && (orionldState.verb != PATCH) && (orionldState.verb != PUT))
  {
    LM_W(("Bad Input (payload body - of %d bytes - for a %s request", orionldState.in.contentLength, verbName(orionldState.verb)));
    orionldErrorResponseCreate(OrionldBadRequestData, "Unexpected payload body", verbName(orionldState.verb));
    orionldState.httpStatusCode = 400;
    goto respond;
  }

  //
  // Any URI param given but not supported?
  // No validity check if OPTIONS verb is used
  //
  if (orionldState.verb != OPTIONS)
  {
    char* detail;
    if (uriParamSupport(orionldState.serviceP->uriParams, orionldState.uriParams.mask, &detail) == false)
    {
      LM_W(("Bad Input (unsupported URI parameter: %s)", detail));
      orionldErrorResponseCreate(OrionldBadRequestData, "Unsupported URI parameter", detail);
      orionldState.httpStatusCode = 400;
      goto respond;
    }
  }

  //
  // If a tenant is used (HTTP Header NGSILD-Tenant) and it's not any of:
  //   * POST /ngsi-ld/v1/entities
  //   * POST /ngsi-ld/v1/entityOperations/create
  //   * POST /ngsi-ld/v1/entityOperations/upsert
  // then if the tenant doesn't exist, an error must be returned (404)
  //
  // The characteristics of the service (if create ot just check for existence) is taken care of by setting (or not)
  // serviceP->options to ORIONLD_SERVICE_OPTION_MAKE_SURE_TENANT_EXISTS in orionldServiceInit.cpp (restServicePrepare)
  //
  if (orionldState.tenantName != NULL)
  {
    if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_MAKE_SURE_TENANT_EXISTS) == ORIONLD_SERVICE_OPTION_MAKE_SURE_TENANT_EXISTS)
    {
      orionldState.tenantP = orionldTenantLookup(orionldState.tenantName);
      if (orionldState.tenantP == NULL)
      {
        //
        // Tenant does not exist in the tenant cache of this broker
        // However, some other broker (load balancer) might have created the tenant!
        //
        if (dbTenantExists(orionldState.tenantName) == false)
        {
          LM_W(("Bad Input (non-existing tenant: '%s')", orionldState.tenantName));
          orionldErrorResponseCreate(OrionldNonExistingTenant, "No such tenant", orionldState.tenantName);
          orionldState.httpStatusCode = 404;
          goto respond;
        }
        else
        {
          // Add tenant to the tenant cache
          orionldState.tenantP = orionldTenantCreate(orionldState.tenantName);
        }
      }
    }
    else
      orionldState.tenantP = orionldTenantGet(orionldState.tenantName);
  }
  else
    orionldState.tenantP = &tenant0;  // No tenant give - default tenant used

  //
  // 03. Check for empty payload for POST/PATCH/PUT
  //
  if (((orionldState.verb == POST) || (orionldState.verb == PATCH) || (orionldState.verb == PUT)) && (payloadEmptyCheck() == false))
    goto respond;


  //
  // Save a copy of the incoming payload before it is destroyed during kjParse AND
  // parse the payload, and check for empty payload, also, find @context in payload and check it's OK
  //
  if (orionldState.in.payload != NULL)
  {
    if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_CLONE_PAYLOAD) == ORIONLD_SERVICE_OPTION_CLONE_PAYLOAD)
      orionldState.in.payloadCopy = kaStrdup(&orionldState.kalloc, orionldState.in.payload);

    if (payloadParseAndExtractSpecialFields(&contextToBeCashed) == false)
      goto respond;
  }

  //
  // 05. Check the Content-Type
  //
  if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_NO_CONTEXT_TYPE_CHECK) == 0)
  {
    if (contentTypeCheck() == false)
      goto respond;
  }

  //
  // 07. Check the @context in HTTP Header, if present
  //
  // NOTE: orionldState.link is set by orionldHttpHeaderReceive() called by orionldMhdConnectionInit()
  //
  // NOTE: Some requests don't use the context and thus should simplt ignore the Link header
  //
  if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED) == 0)
  {
    if ((orionldState.linkHttpHeaderPresent == true) && (linkHeaderCheck() == false))
      goto respond;

    //
    // Treat inline context
    //
    if (orionldState.payloadContextNode != NULL)
    {
      OrionldProblemDetails pd = { OrionldBadRequestData, (char*) "naught", (char*) "naught", 0 };

      char* id  = NULL;
      char* url = NULL;

      //
      // Inline contexts in sub/reg-creation go to the context cache
      // But, not if the context is a simple string, e.g.:
      //   "@context": "https://fiware.github.io/NGSI-LD_TestSuite/ldContext/testContext.jsonld"
      //
      if (((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_CREATE_CONTEXT) != 0) && (orionldState.payloadContextNode->type != KjString))
        url = orionldContextUrlGenerate(&id);

      orionldState.contextP = orionldContextFromTree(url, OrionldContextFromInline, id, orionldState.payloadContextNode, &pd);
      if (orionldState.contextP == NULL)
      {
        LM_W(("Bad Input (invalid context - %s: %s)", pd.title, pd.detail));
        orionldErrorResponseFromProblemDetails(&pd);
        orionldState.httpStatusCode = (HttpStatusCode) pd.status;

        goto respond;
      }

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
  }

  if (orionldState.contextP == NULL)
    orionldState.contextP = orionldCoreContextP;

  if (orionldState.link == NULL)
    orionldState.link = orionldState.contextP->url;


  //
  // @context is in place, even if in the payload body.
  // It is now possible to expand attribute names and entity types for the URI parameters.
  // Only exception is for batch operations that can have a number of @contexts in their entity arrays.
  // But, those operations don't support the affected URI params anyways, so, no probs
  //
  if (uriParamExpansion() == false)
    goto respond;

  // -----------------------------------------------------------------------------
  //
  // Call the SERVICE ROUTINE
  //
  PERFORMANCE(serviceRoutineStart);
 serviceRoutine:
  serviceRoutineResult = orionldState.serviceP->serviceRoutine();
  PERFORMANCE(serviceRoutineEnd);

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
    if (orionldState.pd.status != 0)  // Perhaps the error is in orionldState.pd ?
    {
      orionldErrorResponseCreate(orionldState.pd.type, orionldState.pd.title, orionldState.pd.detail);
      orionldState.httpStatusCode = orionldState.pd.status;
    }
    else
    {
      orionldErrorResponseCreate(OrionldInternalError, "Unknown Error", "The reason for this error is unknown");
      orionldState.httpStatusCode = 500;
    }
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
  //   What if "Accept: application/ld+json" in a creation request?
  //   Creation requests have no payload data in the response so the context can't be put in the payload ...
  //
  // What is clear is that no @context is to be returned for error reponses.
  // Also, if there is no payload data in the response, no need for @context
  // Also, GET /.../contexts/{context-id} should NOT give back the link header
  //
  bool linkHeader = false;

  if ((serviceRoutineResult == true) && (orionldState.noLinkHeader == false) && (orionldState.responseTree != NULL))
  {
    if (orionldState.out.contentType == GEOJSON)
    {
      //
      // Default is: @context in payload body
      // Overridden only if "Prefer: body=json"
      //
      if ((orionldState.preferHeader != NULL) && (strcasecmp(orionldState.preferHeader, "body=json") == 0))
        linkHeader = true;
      else
        orionldState.linkHeaderAdded = false;  // To indicate @context in payload body for geojsonEntityTransform
    }
    else if (orionldState.out.contentType != JSONLD)
      linkHeader = true;
    else if (orionldState.responseTree == NULL)
      linkHeader = true;
  }

  if ((linkHeader == true) && (orionldState.httpStatusCode != 204))
    httpHeaderLinkAdd(orionldState.link);  // sets orionldState.linkHeaderAdded to true => @context in payload body for geojsonEntityTransform

  //
  // Is there a KJSON response tree to render?
  //
  if (orionldState.responseTree != NULL)
  {
    //
    // Should a @context be added to the response payload?
    //
    bool addContext = ((orionldState.serviceP        != NULL)    &&
                       (orionldState.linkHeaderAdded == false)   &&
                       ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD) == 0) &&
                       (orionldState.out.contentType == JSONLD)  &&
                       (orionldState.httpStatusCode   < 300));

    if (addContext)
      contextToPayload();

    //
    // Render the payload to get a string for restReply to send the response
    //
    // FIXME: Smarter allocation !!!
    //
    PERFORMANCE(renderStart);

    if ((orionldState.out.contentType == GEOJSON) && (serviceRoutineResult == true))
    {
      if (orionldState.serviceP->serviceRoutine == orionldGetEntity)
        orionldState.responseTree = kjGeojsonEntityTransform(orionldState.responseTree, orionldState.geoPropertyNode);
      else if (orionldState.serviceP->serviceRoutine == orionldGetEntities)
        orionldState.responseTree = kjGeojsonEntitiesTransform(orionldState.responseTree);
      else
        LM_W(("Bad Input (Accept: application/geo+json for non-compatible request)"));
    }


    //
    // Smart allocation of the response buffer
    //
    // If there is room in the current kalloc biffer (no extra malloc needed), then use it.
    // If not, then it's better to do a separate call to malloc, as:
    //   - the rest of the kalloc buffer isn't thrown away
    //   - the entire logic of kalloc is avoided (not much, but still ...)
    //
    unsigned int responsePayloadSize;

    if (orionldState.uriParams.prettyPrint == false)
      responsePayloadSize = kjFastRenderSize(orionldState.responseTree);
    else
      responsePayloadSize = kjRenderSize(orionldState.kjsonP, orionldState.responseTree);

    if (responsePayloadSize < orionldState.kalloc.bytesLeft + 8)
      orionldState.responsePayload = kaAlloc(&orionldState.kalloc, responsePayloadSize);
    else
    {
      orionldState.responsePayload = (char*) malloc(responsePayloadSize);

      if (orionldState.responsePayload == NULL)
      {
        LM_E(("Out of memory"));
        orionldState.responsePayload = (char*) "{ \"error\": \"Out of memory\"}";
      }
      else
        orionldStateDelayedFreeEnqueue(orionldState.responsePayload);
    }

    if (orionldState.uriParams.prettyPrint == false)
      kjFastRender(orionldState.responseTree, orionldState.responsePayload);
    else
      kjRender(orionldState.kjsonP, orionldState.responseTree, orionldState.responsePayload, responsePayloadSize);

    PERFORMANCE(renderEnd);
  }

  PERFORMANCE(restReplyStart);

  if (orionldState.responsePayload != NULL)
    restReply(NULL, orionldState.responsePayload);    // orionldState.responsePayload freed and NULLed by restReply()
  else
    restReply(NULL, NULL);

  PERFORMANCE(restReplyEnd);

  //
  // FIXME: Delay until requestCompleted. The call to orionldStateRelease as well
  //
  // Call TRoE Routine (if there is one) to save the TRoE data.
  // Only if the Service Routine was successful, of course
  // AND if there is any request tree to process
  //
  if ((orionldState.httpStatusCode >= 200) && (orionldState.httpStatusCode <= 300) && (orionldState.noDbUpdate == false))
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


        //
        // Special case - Entity creation with no attribute
        // As both the entity id and the entity type have been removed from the payload body, the payload body is now empty.
        // We still have to record the creation of the entity in the TRoE database!
        //
        // If the incoming request an empty array/object, then don't call the TRoE routine
        // - EXCEPT if it's a POST /entities request (service routine is orionldPostEntities)
        //
        bool invokeTroe = false;

        if (orionldState.verb == DELETE)                                                                  invokeTroe = true;
        if (orionldState.serviceP->serviceRoutine == orionldPostEntities)                                 invokeTroe = true;
        if ((orionldState.requestTree != NULL) && (orionldState.requestTree->value.firstChildP != NULL))  invokeTroe = true;

        if (invokeTroe == true)
        {
          PERFORMANCE(troeStart);
          orionldState.serviceP->troeRoutine();
          PERFORMANCE(troeEnd);
        }
      }
    }
  }

  //
  // Cleanup
  //
  orionldStateRelease();

  PERFORMANCE(requestPartEnd);

  return MHD_YES;
  }
