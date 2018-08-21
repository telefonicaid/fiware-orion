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
#include "logMsg/logMsg.h"

extern "C"
{
#include "kjson/kjBuilder.h"                              // kjString, kjObject, ...
#include "kjson/kjRender.h"                               // kjRender
}

#include "rest/ConnectionInfo.h"
#include "orionld/context/orionldContextAdd.h"            // Add a context to the context list
#include "orionld/common/orionldErrorResponse.h"          // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                      // SCOMPAREx
#include "orionld/serviceRoutines/orionldPostEntities.h"  // Own interface



// -----------------------------------------------------------------------------
//
// DUPLICATE_CHECK -
//
#define DUPLICATE_CHECK(nodeP, pointer, what)                                                     \
do                                                                                                \
{                                                                                                 \
  if (pointer != NULL)                                                                            \
  {                                                                                               \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Duplicated field", "entity id");      \
    return false;                                                                                 \
  }                                                                                               \
                                                                                                  \
  pointer = nodeP;                                                                                \
                                                                                                  \
} while (0)



// -----------------------------------------------------------------------------
//
// OBJECT_CHECK -
//
#define OBJECT_CHECK(nodeP, what)                                                                 \
do                                                                                                \
{                                                                                                 \
  if (nodeP->type != KjObject)                                                                    \
  {                                                                                               \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON object", what);            \
    return false;                                                                                 \
  }                                                                                               \
} while (0)



// -----------------------------------------------------------------------------
//
// STRING_CHECK -
//
#define STRING_CHECK(nodeP, what)                                                                 \
do                                                                                                \
{                                                                                                 \
  if (nodeP->type != KjString)                                                                    \
  {                                                                                               \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON string", what);            \
    return false;                                                                                 \
  }                                                                                               \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_CHECK -
//
#define ARRAY_CHECK(nodeP, what)                                                                  \
do                                                                                                \
{                                                                                                 \
  if (nodeP->type != KjArray)                                                                     \
  {                                                                                               \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON array", what);             \
    return false;                                                                                 \
  }                                                                                               \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_OR_STRING_CHECK -
//
#define ARRAY_OR_STRING_CHECK(nodeP, what)                                                        \
do                                                                                                \
{                                                                                                 \
  if ((nodeP->type != KjArray) && (nodeP->type != KjString))                                      \
  {                                                                                               \
    LM_TMP(("the node is a '%s'", kjValueType(nodeP->type)));                                     \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON array nor string", what);  \
    return false;                                                                                 \
  }                                                                                               \
} while (0)



// -----------------------------------------------------------------------------
//
// httpHeaderLocationAdd -
//
void httpHeaderLocationAdd(ConnectionInfo* ciP, const char* uriPathWithSlash, const char* entityId)
{
  char location[512];

  snprintf(location, sizeof(location), "%s%s", uriPathWithSlash, entityId);

  ciP->httpHeader.push_back(HTTP_RESOURCE_LOCATION);
  ciP->httpHeaderValue.push_back(location);
}



// ----------------------------------------------------------------------------
//
// contextItemNodeTreat -
//
static bool contextItemNodeTreat(ConnectionInfo* ciP, char* url)
{
  char* details = NULL;

  bool b = orionldContextAdd(ciP, url, &details);
  if (b == false)
  {
    LM_E(("Invalid context '%s': %s", url, details));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", details);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// payloadCheck -
//
static bool payloadCheck(ConnectionInfo* ciP, KjNode** idNodePP, KjNode** typeNodePP, KjNode** contextNodePP)
{
  OBJECT_CHECK(ciP->requestTopP, "toplevel");

  KjNode*  kNodeP        = ciP->requestTopP->children;
  KjNode*  idNodeP       = NULL;
  KjNode*  typeNodeP     = NULL;
  KjNode*  contextNodeP  = NULL;

  //
  // First make sure all mandatory data is present and that data types are correct
  //
  while (kNodeP != NULL)
  {
    if (SCOMPARE3(kNodeP->name, 'i', 'd', 0))
    {
      DUPLICATE_CHECK(kNodeP, idNodeP, "entity id");
      STRING_CHECK(kNodeP, "entity id");
    }
    else if (SCOMPARE5(kNodeP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(kNodeP, typeNodeP, "entity type");
      STRING_CHECK(kNodeP, "entity type");
    }
    else if (SCOMPARE9(kNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      DUPLICATE_CHECK(kNodeP, contextNodeP, "context");
      ARRAY_OR_STRING_CHECK(kNodeP, "@context");
    }
    
    kNodeP = kNodeP->next;
  }

  //
  // Check presense of mandatory fields
  //
  if (idNodeP == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "No 'id' of the entity", "The 'id' field is mandatory");
    return false;
  }

  //
  // Prepare output
  //
  *idNodePP      = idNodeP;
  *typeNodePP    = typeNodeP;
  *contextNodePP = contextNodeP;

  return true;
}



// -----------------------------------------------------------------------------
//
// attributeTreat -
//
static bool attributeTreat(ConnectionInfo* ciP, KjNode* kNodeP)
{
  char*   attrName = kNodeP->name;
  
  LM_TMP(("Treating attribute '%s' (KjNode at %p)", attrName, kNodeP));

  OBJECT_CHECK(kNodeP, "attribute");

  KjNode* typeP        = NULL;
  KjNode* valueP       = NULL;
  KjNode* unitCodeP    = NULL;
  KjNode* objectP      = NULL;
  KjNode* providedByP  = NULL;
  KjNode* reliabilityP = NULL;
  KjNode* nodeP        = kNodeP->children;

  while (nodeP != NULL)
  {
    LM_TMP(("Treating part '%s' of attribute '%s'", nodeP->name, attrName));

    if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, typeP, "attribute type");
      STRING_CHECK(nodeP, "attribute type");
    }
    else if (SCOMPARE6(nodeP->name, 'v', 'a', 'l', 'u', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, valueP, "attribute type");
    }
    else if (SCOMPARE9(nodeP->name, 'u', 'n', 'i', 't', 'C', 'o', 'd', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, unitCodeP, "unit code");
    }
    else if (SCOMPARE7(nodeP->name, 'o', 'b', 'j', 'e', 'c', 't', 0))
    {
      DUPLICATE_CHECK(nodeP, objectP, "object");
    }
    else if (SCOMPARE11(nodeP->name, 'p', 'r', 'o', 'v', 'i', 'd', 'e', 'd', 'B', 'y', 0))
    {
      DUPLICATE_CHECK(nodeP, providedByP, "provided by");
    }
    else if (SCOMPARE12(nodeP->name, 'r', 'e', 'l', 'i', 'a', 'b', 'i', 'l', 'i', 't', 'y', 0))
    {
      DUPLICATE_CHECK(nodeP, reliabilityP, "reliability");
    }
    else  // Other
    {
      // bool isProperty  = false;

      // Valid types: "Property" and "Relationship"
      char* type = typeP->value.s;

      if (strcmp(type, "Property") == 0)
      {
        // isProperty = true;
      }
      else if (strcmp(type, "Relationship") == 0)
      {
        // isProperty = false;
      }
      else
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid type for attribute", type);
        return false;
      }
    }

    nodeP = nodeP->next;
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPostEntities -
//
bool orionldPostEntities(ConnectionInfo* ciP)
{
  KjNode* idNodeP      = NULL;
  KjNode* typeNodeP    = NULL;
  KjNode* contextNodeP = NULL;
  KjNode* kNodeP;

  if (payloadCheck(ciP, &idNodeP, &typeNodeP, &contextNodeP) == false)
    return false;


  // Treat the entire payload
  for (kNodeP = ciP->requestTopP->children; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (kNodeP == idNodeP)
    {
      continue;
    }
    else if (kNodeP == typeNodeP)
    {
      continue;
    }
    else if (kNodeP == contextNodeP)
    {
      //
      // In the first implementation of ngsi-ld, the allowed payloads for the @context member are:
      //
      // 1. An array of URL strings:
      //    "@context": [
      //      "http://...",
      //      "http://...",
      //      "http://..."
      //    }
      //
      // 2. A single URL string:
      //    "@context": "http://..."
      //
      // As the payload is already parsed, what needs to be done here is to call orionldContextAdd() for each of these URLs
      //
      // The content of these "Context URLs" can be:
      //
      // 1. An object with a single member '@context' that is an object containing key-values:
      //    "@context" {
      //      "Property": "http://...",
      //      "XXX";      ""
      //    }
      //
      // 2. An object with a single member '@context', that is a vector of URL strings (https://fiware.github.io/NGSI-LD_Tests/ldContext/testFullContext.jsonld):
      //    {
      //      "@context": [
      //        "http://...",
      //        "http://...",
      //        "http://..."
      //      }
      //    }
      //
      LM_TMP(("Got a @context, of type %s", kjValueType(kNodeP->type)));

      if (kNodeP->type == KjString)
      {
        LM_TMP(("The context is a STRING"));
        if (contextItemNodeTreat(ciP, kNodeP->value.s) == false)
          return false;
      }
      else if (kNodeP->type == KjArray)
      {
        LM_TMP(("The context is a VECTOR OF STRINGS"));
        for (KjNode* contextItemNodeP = kNodeP->children; contextItemNodeP != NULL; contextItemNodeP = contextItemNodeP->next)
        {
          if (contextItemNodeTreat(ciP, contextItemNodeP->value.s) == false)
          {
            return false;
          }
        }
      }
      else if (kNodeP->type == KjObject)
      {
        // FIXME: seems like an inline context - not supported for now
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", "inline contexts not supported in current version of orionld");
        return false;
      }
      else
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", "invalid JSON type of @context member");
        return false;
      }
    }
    else  // Must be an attribute
    {
      LM_TMP(("Not id/type/@context: '%s' - treating as attribute", kNodeP->name));

      if (attributeTreat(ciP, kNodeP) == false)
        return false;
    }
  }

  ciP->httpStatusCode  = SccCreated;  // No payload
  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/entities/", idNodeP->value.s);

  return true;
}
