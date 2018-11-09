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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/kjBuilder.h"                                   // kjString, kjObject, ...
#include "kjson/kjRender.h"                                    // kjRender
}

#include "common/globals.h"                                    // parse8601Time
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/rest.h"                                         // restPortGet
#include "orionTypes/OrionValueType.h"                         // orion::ValueType
#include "orionTypes/UpdateActionType.h"                       // ActionType
#include "parse/CompoundValueNode.h"                           // CompoundValueNode
#include "ngsi/ContextAttribute.h"                             // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                       // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                      // UpdateContextResponse
#include "mongoBackend/mongoEntityExists.h"                    // mongoEntityExists
#include "mongoBackend/mongoUpdateContext.h"                   // mongoUpdateContext

#include "orionld/rest/orionldServiceInit.h"                   // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextAdd.h"                 // Add a context to the context list
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextCreateFromUrl.h"       // orionldContextCreateFromUrl
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextItemLookup.h"          // orionldContextItemLookup
#include "orionld/context/orionldContextList.h"                // orionldContextHead, orionldContextTail
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldUserContextKeyValuesCheck.h"  // orionldUserContextKeyValuesCheck
#include "orionld/serviceRoutines/orionldPostEntities.h"       // Own interface



// -----------------------------------------------------------------------------
//
// DUPLICATE_CHECK -
//
#define DUPLICATE_CHECK(nodeP, pointer, what)                                                               \
do                                                                                                          \
{                                                                                                           \
  if (pointer != NULL)                                                                                      \
  {                                                                                                         \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Duplicated field", what, OrionldDetailsString); \
    return false;                                                                                           \
  }                                                                                                         \
                                                                                                            \
  pointer = nodeP;                                                                                          \
                                                                                                            \
} while (0)



// -----------------------------------------------------------------------------
//
// OBJECT_CHECK -
//
#define OBJECT_CHECK(nodeP, what)                                                                            \
do                                                                                                           \
{                                                                                                            \
  if (nodeP->type != KjObject)                                                                               \
  {                                                                                                          \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON object", what, OrionldDetailsString); \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// ATTRIBUTE_IS_OBJECT_CHECK -
//
#define ATTRIBUTE_IS_OBJECT_CHECK(nodeP)                                                                                          \
do                                                                                                                                \
{                                                                                                                                 \
  if (nodeP->type != KjObject)                                                                                                    \
  {                                                                                                                               \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Attribute must be a JSON object", nodeP->name, OrionldDetailsString); \
    return false;                                                                                                                 \
  }                                                                                                                               \
} while (0)



// -----------------------------------------------------------------------------
//
// STRING_CHECK -
//
#define STRING_CHECK(nodeP, what)                                                                            \
do                                                                                                           \
{                                                                                                            \
  if (nodeP->type != KjString)                                                                               \
  {                                                                                                          \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON string", what, OrionldDetailsString); \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_CHECK -
//
#define ARRAY_CHECK(nodeP, what)                                                                            \
do                                                                                                          \
{                                                                                                           \
  if (nodeP->type != KjArray)                                                                               \
  {                                                                                                         \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON array", what, OrionldDetailsString); \
    return false;                                                                                           \
  }                                                                                                         \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_OR_STRING_CHECK -
//
#define ARRAY_OR_STRING_CHECK(nodeP, what)                                                                             \
do                                                                                                                     \
{                                                                                                                      \
  if ((nodeP->type != KjArray) && (nodeP->type != KjString))                                                           \
  {                                                                                                                    \
    LM_T(LmtPayloadCheck, ("the node is a '%s'", kjValueType(nodeP->type)));                                           \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON array nor string", what, OrionldDetailsString); \
    return false;                                                                                                      \
  }                                                                                                                    \
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
// httpHeaderLinkAdd -
//
void httpHeaderLinkAdd(ConnectionInfo* ciP, char* url)
{
  ciP->httpHeader.push_back(HTTP_LINK);
  ciP->httpHeaderValue.push_back(url);
}



// ----------------------------------------------------------------------------
//
// contextItemNodeTreat -
//
static OrionldContext* contextItemNodeTreat(ConnectionInfo* ciP, char* url)
{
  char*            details;
  OrionldContext*  contextP = orionldContextAdd(ciP, url, OrionldUserContext, &details);

  if (contextP == NULL)
  {
    LM_E(("Invalid context '%s': %s", url, details));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", details, OrionldDetailsString);
    return NULL;
  }

  return contextP;
}



// -----------------------------------------------------------------------------
//
// stringContentCheck -
//
static bool stringContentCheck(char* name, char** detailsPP)
{
  if (name == NULL)
  {
    *detailsPP = (char*) "empty name";
    return false;
  }

  for (; *name != 0; ++name)
  {
    //
    // Valid chars:
    //   o a-z: 97-122
    //   o A-Z: 65-90
    //   o 0-9: 48-57
    //   o '_': 95
    //

    if ((*name >= 'a') && (*name <= 'z'))
      continue;
    if ((*name >= 'A') && (*name <= 'Z'))
      continue;
    if ((*name >= '0') && (*name <= '9'))
      continue;
    if (*name == '_')
      continue;

    *detailsPP = (char*) "invalid character in name";
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// payloadCheck -
//
static bool payloadCheck
(
  ConnectionInfo*  ciP,
  KjNode**         idNodePP,
  KjNode**         typeNodePP,
  KjNode**         locationNodePP,
  KjNode**         contextNodePP,
  KjNode**         observationSpaceNodePP,
  KjNode**         operationSpaceNodePP
)
{
  OBJECT_CHECK(ciP->requestTree, "toplevel");

  KjNode*  kNodeP                 = ciP->requestTree->children;
  KjNode*  idNodeP                = NULL;
  KjNode*  typeNodeP              = NULL;
  KjNode*  contextNodeP           = NULL;

  KjNode*  locationNodeP          = NULL;
  KjNode*  observationSpaceNodeP  = NULL;
  KjNode*  operationSpaceNodeP    = NULL;

  //
  // First make sure all mandatory data is present and that data types are correct
  //
  while (kNodeP != NULL)
  {
    char* detailsP;

    if (SCOMPARE3(kNodeP->name, 'i', 'd', 0))
    {
      DUPLICATE_CHECK(kNodeP, idNodeP, "entity id");
      STRING_CHECK(kNodeP, "entity id");
    }
    else if (SCOMPARE5(kNodeP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(kNodeP, typeNodeP, "entity type");
      STRING_CHECK(kNodeP, "entity type");
      if (stringContentCheck(kNodeP->value.s, &detailsP) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid entity type name", detailsP, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE9(kNodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(kNodeP, locationNodeP, "location");
      // FIXME: check validity of location - GeoProperty
    }
    else if (SCOMPARE9(kNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      DUPLICATE_CHECK(kNodeP, contextNodeP, "context");
      ARRAY_OR_STRING_CHECK(kNodeP, "@context");
    }
    else if (SCOMPARE17(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(kNodeP, observationSpaceNodeP, "context");
      // FIXME: check validity of observationSpace - GeoProperty
    }
    else if (SCOMPARE15(kNodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(kNodeP, operationSpaceNodeP, "context");
      // FIXME: check validity of operationSpaceP - GeoProperty
    }
    else  // Property/Relationshiop - must check chars in the name of the attribute
    {
      // FIXME: Make sure the type is either Property or Relationship
      if (stringContentCheck(kNodeP->name, &detailsP) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Property/Relationship name", detailsP, OrionldDetailsString);
        return false;
      }
    }

    kNodeP = kNodeP->next;
  }

  //
  // Check presence of mandatory fields
  //
  if (idNodeP == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "No 'id' of the entity", "The 'id' field is mandatory", OrionldDetailsString);
    return false;
  }

  if (typeNodeP == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "No 'type' of the entity", "The 'type' field is mandatory", OrionldDetailsString);
    return false;
  }


  //
  // Prepare output
  //
  *idNodePP               = idNodeP;
  *typeNodePP             = typeNodeP;
  *locationNodePP         = locationNodeP;
  *contextNodePP          = contextNodeP;
  *observationSpaceNodePP = observationSpaceNodeP;
  *operationSpaceNodePP   = operationSpaceNodeP;

  return true;
}



// ----------------------------------------------------------------------------
//
// uriExpansion -
//
// 1. Lookup in user context, expand if found
// 2. Lookup in Core Context - if found there, no expansion is made, just return from function
// 3. Expand using the default context
//
// Returns the number of expansions done.
//  -2: expansion NOT found in the context - use default URL
//  -1: an error has occurred
//   0: found in Core context only. No expansion is to be made
//   1: expansion found only for the attr-name/entity-type
//   2: expansions found both for attr-name and attr-type
//
static int uriExpansion(OrionldContext* contextP, const char* name, char** expandedNameP, char** expandedTypeP, char** detailsPP)
{
  KjNode* contextValueP = NULL;

  *expandedNameP = NULL;
  *expandedTypeP = NULL;

  LM_T(LmtUriExpansion, ("looking up alias for '%s'", name));

  //
  // Use the default context?
  // Two possibilities for default context:
  // 1. No user context present
  // 2. Not found in user context
  //
  // But, if found in Core Context, then NO Expansion is to be made
  //
  if (contextP != NULL)
  {
    LM_T(LmtUriExpansion, ("looking up context item for '%s', in context '%s'", name, contextP->url));
    contextValueP = orionldContextItemLookup(contextP, name);
  }

  if (contextValueP == NULL)
  {
    //
    // Not found.
    // Now, if found in Core context, no expansion is to be made (return 0).
    // If not, then the default URL should be used (return 3).
    //
    contextValueP = orionldContextItemLookup(&orionldCoreContext, name);

    if (contextValueP != NULL)
      return 0;
    else
      return -2;
  }


  //
  // Context Item found - must be either a string or an object containing two strings
  //
  LM_T(LmtUriExpansion, ("contextValueP at %p", contextValueP));
  if (contextValueP->type == KjString)
  {
    LM_T(LmtUriExpansion, ("got a string - expanded name is '%s'", contextValueP->value.s));
    *expandedNameP = contextValueP->value.s;

    return 1;
  }

  if (contextValueP->type != KjObject)
  {
    // FIXME: I need ciP here to fill in the error-response
    return -1;
  }

  //
  // The context item has a complex value: "@id" and "@type".
  // The value of:
  // - "@id":     corresponds to the 'name' of the attribute
  // - "@type":   corresponds to the 'type' of the attribute??? FIXME
  //
  int children = 0;
  for (KjNode* nodeP = contextValueP->children; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE4(nodeP->name, '@', 'i', 'd', 0))
    {
      *expandedNameP = nodeP->value.s;
      LM_T(LmtUriExpansion, ("got an object - expanded name is '%s'", nodeP->value.s));
    }
    else if (SCOMPARE6(nodeP->name, '@', 't', 'y', 'p', 'e', 0))
    {
      *expandedTypeP = nodeP->value.s;
      LM_T(LmtUriExpansion, ("got an object - expanded type is '%s'", nodeP->value.s));
    }
    else
    {
      *detailsPP = (char*) "Invalid context - invalid field in context item object";
      LM_E(("uriExpansion: Invalid context - invalid field in context item object: '%s'", nodeP->name));
      return -1;
    }
    ++children;
  }

  //
  // If an expansion has been found, we MUST have a "@id", if not - ERROR
  //
  if ((children >= 1) && (*expandedNameP == NULL))
  {
    *detailsPP = (char*) "Invalid context - no @id in complex context item";
    return -1;
  }


  //
  // FIXME: Is this assumption true?
  //        If the value of a @context item is an object, must it have exactly TWO members, @id and @type?
  //
  // FIXME: This check should NOT be done here, every time, but ONCE when the context is first downloaded
  //
  if ((children != 2) || (*expandedNameP == NULL) || (*expandedTypeP == NULL))
  {
    *detailsPP = (char*) "Invalid context - field in context item object not matching the rules";
    LM_E(("uriExpansion: invalid @context item '%s' in @context '%s'", name, contextP->url));
    return -1;
  }

  LM_T(LmtUriExpansion, ("returning %d (expansions found): name='%s', type='%s'", children, *expandedNameP, *expandedTypeP));
  return children;
}



// -----------------------------------------------------------------------------
//
// compoundCreate -
//
static orion::CompoundValueNode* compoundCreate(ConnectionInfo* ciP, KjNode* kNodeP, KjNode* parentP, int level = 0)
{
  if (kNodeP->type != KjArray)
    LM_T(LmtCompoundCreation, ("In compoundCreate: creating '%s' called '%s' on level %d", kjValueType(kNodeP->type), kNodeP->name, level));
  else
    LM_T(LmtCompoundCreation, ("In compoundCreate: creating '%s' on level %d", kjValueType(kNodeP->type), level));

  orion::CompoundValueNode* cNodeP = new orion::CompoundValueNode();

  if ((parentP != NULL) && (parentP->type == KjObject))
    cNodeP->name = kNodeP->name;

#if 0
  // Any URI Expansion needed?
  if ((kNodeP->name != NULL) && (kNodeP->name[0] != 0))
  {
    char* expandedNameP  = NULL;
    char* expandedTypeP  = NULL;
    char* details        = NULL;
    int   expansions;

    expansions = uriExpansion(ciP->contextP, kNodeP->name, &expandedNameP, &expandedTypeP, &details);
    if (expansions == -1)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context item", details, OrionldDetailsString);
      return NULL;
    }

    if (expandedNameP != NULL)
    {
      LM_T(LmtUriExpansion, ("Expansion found for %s name '%s': %s", kjValueType(kNodeP->type), kNodeP->name, expandedNameP));
      cNodeP->name = expandedNameP;
    }
    else
    {
      LM_T(LmtUriExpansion, ("NO Expansion found for %s name '%s': using: %s%s", kjValueType(kNodeP->type), kNodeP->name, orionldDefaultUrl, kNodeP->name));
      cNodeP->name = std::string(orionldDefaultUrl) + kNodeP->name;
    }
  }
#endif

  if (kNodeP->type == KjString)
  {
    cNodeP->valueType   = orion::ValueTypeString;
    cNodeP->stringValue = kNodeP->value.s;
  }
  else if (kNodeP->type == KjBoolean)
  {
    cNodeP->valueType   = orion::ValueTypeBoolean;
    cNodeP->boolValue   = kNodeP->value.b;
  }
  else if (kNodeP->type == KjFloat)
  {
    cNodeP->valueType   = orion::ValueTypeNumber;
    cNodeP->numberValue = kNodeP->value.f;
  }
  else if (kNodeP->type == KjInt)
  {
    cNodeP->valueType   = orion::ValueTypeNumber;
    cNodeP->numberValue = kNodeP->value.i;
  }
  else if (kNodeP->type == KjNull)
  {
    cNodeP->valueType = orion::ValueTypeNull;
  }
  else if (kNodeP->type == KjObject)
  {
    ++level;
    cNodeP->valueType = orion::ValueTypeObject;

    for (KjNode* kChildP = kNodeP->children; kChildP != NULL; kChildP = kChildP->next)
    {
      orion::CompoundValueNode* cChildP = compoundCreate(ciP, kChildP, kNodeP, level);
      cNodeP->childV.push_back(cChildP);
    }
  }
  else if (kNodeP->type == KjArray)
  {
    ++level;
    cNodeP->valueType = orion::ValueTypeVector;

    for (KjNode* kChildP = kNodeP->children; kChildP != NULL; kChildP = kChildP->next)
    {
      orion::CompoundValueNode* cChildP = compoundCreate(ciP, kChildP, kNodeP, level);

      cNodeP->childV.push_back(cChildP);
    }
  }

  return cNodeP;
}



// -----------------------------------------------------------------------------
//
// GeoJSON -
//
// The value of a GeoJSON Property must be a JSON Object.
// The Object must have exactly two members:
// - type:        a JSON String that is a supported GeoJSON type (Point, Polygon, ...)
// - coordinates: a JSON Array of Numbers. The number of members depends on the 'type'
//
//
// ACCEPTED TYPES:
// First of all, a Position is an Array of 2-3 Numbers  (Implementations SHOULD NOT extend positions beyond three elements)
//
//   Point:            a Position                        [ 1, 2 ]
//   MultiPoint:       an Array of Positions             [ [ 1, 2 ], [ 1, 2 ], [ 1, 2 ], ... ]
//   LineString:       an Array of 2+ Positions          [ [ 1, 2 ], [ 1, 2 ], ... ]
//   MultiLineString:  an Array of LineString            [ [ [ 1, 2 ], [ 1, 2 ] ], [ [ 1, 2 ], [ 1, 2 ] ], ... ]
//   Polygon:          closed LineString with 4+ pos     [ [ 1, 2 ], [ 3, 4 ], [ 5, 6 ], ..., [ 1, 2 ] ]
//   MultiPolygon:     an Array of Polygon               [ [ [ [ 1, 2 ], [ 1, 2 ] ], [ [ 1, 2 ], [ 1, 2 ] ], ... ], [ [ [ 1, 2 ], [ 1, 2 ] ], [ [ 1, 2 ], [ 1, 2 ] ], ... ], ... ]
//



// -----------------------------------------------------------------------------
//
// GeoJsonType -
//
typedef enum GeoJsonType
{
  GeoJsonPoint,
  GeoJsonMultiPoint,
  GeoJsonLineString,
  GeoJsonMultiLineString,
  GeoJsonPolygon,
  GeoJsonMultiPolygon
} GeoJsonType;



// -----------------------------------------------------------------------------
//
// geoJsonTypeCheck -
//
static bool geoJsonTypeCheck(char* typeName, GeoJsonType* typeP, char** detailsP)
{
  if      (SCOMPARE6(typeName, 'P', 'o', 'i', 'n', 't', 0))                                                       *typeP = GeoJsonPoint;
  else if (SCOMPARE11(typeName, 'M', 'u', 'l', 't', 'i', 'P', 'o', 'i', 'n', 't', 0))                             *typeP = GeoJsonMultiPoint;
  else if (SCOMPARE11(typeName, 'L', 'i', 'n', 'e', 'S', 't', 'r', 'i', 'n', 'g', 0))                             *typeP = GeoJsonLineString;
  else if (SCOMPARE16(typeName, 'M', 'u', 'l', 't', 'i', 'L', 'i', 'n', 'e', 'S', 't', 'r', 'i', 'n', 'g', 0))    *typeP = GeoJsonLineString;
  else if (SCOMPARE8(typeName, 'P', 'o', 'l', 'y', 'g', 'o', 'n', 0))                                             *typeP = GeoJsonPolygon;
  else if (SCOMPARE13(typeName, 'M', 'u', 'l', 't', 'i', 'P', 'o', 'l', 'y', 'g', 'o', 'n', 0))                   *typeP = GeoJsonMultiPolygon;
  else
  {
    LM_E(("Invalid GeoJSON type: %s", typeName));
    *detailsP = (char*) "invalid GeoJSON type";

    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// geojsonCheck - check validity of any geo-property
//
static bool geojsonCheck(ConnectionInfo* ciP, KjNode* geoJsonNodeP, char** detailsP)
{
  KjNode*     typeP    = NULL;
  KjNode*     coordsP  = NULL;
  GeoJsonType geoJsonType  = (GeoJsonType) -1;

  if (geoJsonNodeP->type != KjObject)
  {
    *detailsP = (char*) "A GeoProperty value must be a JSON Object";
    return false;
  }

  for (KjNode* itemP = geoJsonNodeP->children; itemP != NULL; itemP = itemP->next)
  {
    LM_TMP(("Item '%s'", itemP->name));

    if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(itemP, typeP, geoJsonNodeP->name);

      if (itemP->type != KjString)
      {
        *detailsP = (char*) "the 'type' field of a GeoJSON object must be a JSON String";
        return false;
      }

      if (geoJsonTypeCheck(itemP->value.s, &geoJsonType, detailsP) == false)
        return false;
    }
    else if (SCOMPARE12(itemP->name, 'c', 'o', 'o', 'r', 'd', 'i', 'n', 'a', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK(itemP, coordsP, geoJsonNodeP->name);

      if (itemP->type != KjArray)
      {
        *detailsP = (char*) "the 'coordinates' field of a GeoJSON object must be a JSON Array";
        return false;
      }
    }
    else
    {
      *detailsP = (char*) "invalid field in a GeoJSON object";
      return false;
    }
  }

  if (typeP == NULL)
  {
    *detailsP = (char*) "Mandatory 'type' field missing for a GeoJSON Property";
    return false;
  }

  if (coordsP == NULL)
  {
    *detailsP = (char*) "Mandatory 'coordinates' field missing for a GeoJSON Property";
    return false;
  }

  // FIXME: Check all types of GeoJSON - Point, Polygon, etc (different Arrays of Number)
  // if (geoJsonCoordinatesCheck(coordsP->children, detailsP) == false)
  //   return false;

  return true;
}



// -----------------------------------------------------------------------------
//
// metadataAdd -
//
static bool metadataAdd(ConnectionInfo* ciP, ContextAttribute* caP, KjNode* nodeP, char* caName)
{
  LM_TMP(("Create metadata '%s' (a JSON %s) and add to attribute '%s'", nodeP->name, kjValueType(nodeP->type), caName));

  KjNode*   typeNodeP       = NULL;
  KjNode*   valueNodeP      = NULL;
  KjNode*   objectNodeP     = NULL;
  bool      isProperty      = false;
  bool      isRelationship  = false;

  if (nodeP->type == KjObject)
  {
    for (KjNode* kNodeP = nodeP->children; kNodeP != NULL; kNodeP = kNodeP->next)
    {
      if (SCOMPARE5(kNodeP->name, 't', 'y', 'p', 'e', 0))
      {
        DUPLICATE_CHECK(kNodeP, typeNodeP, "metadata type");
        STRING_CHECK(kNodeP, "metadata type");

        if (SCOMPARE9(kNodeP->value.s, 'P', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
        {
          isProperty = true;
        }
        else if (SCOMPARE13(kNodeP->value.s, 'R', 'e', 'l', 'a', 't', 'i', 'o', 'n', 's', 'h', 'i', 'p', 0))
        {
          isRelationship = true;
        }
        else
        {
          LM_E(("Invalid type for metadata '%s': '%s'", kNodeP->name, kNodeP->value.s));
          orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid type for attribute", kNodeP->value.s, OrionldDetailsString);
          return false;
        }

        typeNodeP = kNodeP;
      }
      else if (SCOMPARE6(kNodeP->name, 'v', 'a', 'l', 'u', 'e', 0))
      {
        DUPLICATE_CHECK(kNodeP, valueNodeP, "metadata value");
        valueNodeP = kNodeP;
      }
      else if (SCOMPARE7(kNodeP->name, 'o', 'b', 'j', 'e', 'c', 't', 0))
      {
        DUPLICATE_CHECK(kNodeP, objectNodeP, "metadata object");
        objectNodeP = kNodeP;
      }
    }

    if (typeNodeP == NULL)
    {
      LM_E(("No 'type' for metadata '%s'", nodeP->name));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "The field 'type' is missing for a metadata", nodeP->name, OrionldDetailsString);
      return false;
    }

    if ((isProperty == true) && (valueNodeP == NULL))
    {
      LM_E(("No 'value' for Property metadata '%s'", nodeP->name));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "The field 'value' is missing for a Property metadata", nodeP->name, OrionldDetailsString);
      return false;
    }
    else if ((isRelationship == true) && (objectNodeP == NULL))
    {
      LM_E(("No 'object' for Relationship metadata '%s'", nodeP->name));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "The field 'value' is missing for a Relationship metadata", nodeP->name, OrionldDetailsString);
      return false;
    }
  }
  else
  {
    isProperty = true;
    valueNodeP = nodeP;
  }

  Metadata* mdP = new Metadata();

  if (mdP == NULL)
  {
    LM_E(("out of memory creating property/relationship '%s' for attribute '%s'", nodeP->name, caName));
    orionldErrorResponseCreate(ciP, OrionldInternalError, "cannot create property/relationship for attribute", "out of memory", OrionldDetailsString);
    return false;
  }

  mdP->name = nodeP->name;

  if (typeNodeP != NULL)  // Only if the metadata is a JSON Object
    mdP->type = typeNodeP->value.s;

  if (isProperty == true)
  {
    switch (valueNodeP->type)
    {
    case KjBoolean:    mdP->valueType = orion::ValueTypeBoolean; mdP->boolValue      = valueNodeP->value.b; break;
    case KjInt:        mdP->valueType = orion::ValueTypeNumber;  mdP->numberValue    = valueNodeP->value.i; break;
    case KjFloat:      mdP->valueType = orion::ValueTypeNumber;  mdP->numberValue    = valueNodeP->value.f; break;
    case KjString:     mdP->valueType = orion::ValueTypeString;  mdP->stringValue    = valueNodeP->value.s; break;
    case KjObject:     mdP->valueType = orion::ValueTypeObject;  mdP->compoundValueP = compoundCreate(ciP, valueNodeP, NULL); break;
    case KjArray:      mdP->valueType = orion::ValueTypeObject;  mdP->compoundValueP = compoundCreate(ciP, valueNodeP, NULL);  break;
    case KjNull:       mdP->valueType = orion::ValueTypeNull;    break;
    case KjNone:
      LM_E(("Invalid json type (KjNone!) for value field of metadata '%s'", nodeP->name));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal error", "Invalid type from kjson", OrionldDetailsString);
      delete mdP;
      return false;
    }
  }
  else  // Relationship
  {
    // A "Relationship" has no 'value', instead it has 'object', that must be of string type
    if (objectNodeP->type != KjString)
    {
      LM_E(("invalid json type for relationship-object '%s' of attribute '%s'", nodeP->name, caName));
      orionldErrorResponseCreate(ciP, OrionldInternalError, "invalid json type for relationship-object", nodeP->name, OrionldDetailsString);
      delete mdP;
      return false;
    }

    mdP->valueType   = orion::ValueTypeString;
    mdP->stringValue = objectNodeP->value.s;
  }

  caP->metadataVector.push_back(mdP);

  return true;
}



// -----------------------------------------------------------------------------
//
// ATTRIBUTE_ERROR -
//
#define ATTRIBUTE_ERROR(errorString, details)                                                            \
do                                                                                                       \
{                                                                                                        \
  LM_E((errorString));                                                                                   \
  orionldErrorResponseCreate(ciP, OrionldBadRequestData, errorString, details, OrionldDetailsAttribute); \
  return false;                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// attributeTreat -
//
// We will need more than one function;
// - propertyTreat
// - geoPropertyTreat ?
// - relationshipTreat
//
static bool attributeTreat(ConnectionInfo* ciP, KjNode* kNodeP, ContextAttribute* caP, KjNode** typeNodePP)
{
  char* caName = kNodeP->name;

  LM_T(LmtPayloadCheck, ("Treating attribute '%s' (KjNode at %p)", kNodeP->name, kNodeP));

  ATTRIBUTE_IS_OBJECT_CHECK(kNodeP);

  //
  // For performance issues, all predefined names should have their char-sum precalculated
  //
  // E.g.:
  // const int TYPE_CHARSUM = 't' + 'y' + 'p' + 'e'
  //
  // int nodeNameCharsum = 0;
  // for (char* nodeNameP = nodeP->name; *nodeNameP != 0; ++nodeNameP)
  //   nodeNameCharsum += *nodeNameP;
  //
  // if ((nodeNameCharsum == TYPE_CHARSUM) && (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0)))
  // { ... }
  //
  // ADVANTAGE:
  //   Just a simple integer comparison before we do the complete string-comparisom
  //
  KjNode* typeP              = NULL;  // For ALL:            Mandatory
  KjNode* valueP             = NULL;  // For 'Property':     Mandatory
  KjNode* objectP            = NULL;  // For 'Relationship:  Mandatory
  KjNode* unitCodeP          = NULL;  // For 'Property':     Optional
  KjNode* observedAtP        = NULL;  // For ALL:            Optional
  KjNode* observationSpaceP  = NULL;  // For 'GeoProperty':  Optional
  KjNode* operationSpaceP    = NULL;  // For 'GeoProperty':  Optional
  bool    isProperty         = false;
  bool    isGeoProperty      = false;
  bool    isTemporalProperty = false;
  bool    isRelationship     = false;
  KjNode* nodeP              = kNodeP->children;

  while (nodeP != NULL)
  {
    LM_T(LmtPayloadCheck, ("Treating part '%s' of attribute '%s'", nodeP->name, kNodeP->name));

    if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, typeP, "attribute type");
      STRING_CHECK(nodeP, "attribute type");

      if (SCOMPARE9(nodeP->value.s, 'P', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
      {
        isProperty = true;
      }
      else if (SCOMPARE13(nodeP->value.s, 'R', 'e', 'l', 'a', 't', 'i', 'o', 'n', 's', 'h', 'i', 'p', 0))
      {
        isRelationship = true;
      }
      else if (SCOMPARE12(nodeP->value.s, 'G', 'e', 'o', 'P', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
      {
        isProperty    = true;
        isGeoProperty = true;
      }
      else if (SCOMPARE17(nodeP->value.s, 'T', 'e', 'm', 'p', 'o', 'r', 'a', 'l', 'P', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
      {
        isProperty         = true;
        isTemporalProperty = false;
      }
      else
      {
        LM_E(("Invalid type for attribute '%s': '%s'", nodeP->name, nodeP->value.s));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid type for attribute", nodeP->value.s, OrionldDetailsString);
        return false;
      }

      *typeNodePP = typeP;
    }
    else if (SCOMPARE6(nodeP->name, 'v', 'a', 'l', 'u', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, valueP, "attribute value"); 
      // FIXME: "value" for Relationship Attribute should be added as metadata
   }
    else if (SCOMPARE9(nodeP->name, 'u', 'n', 'i', 't', 'C', 'o', 'd', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, unitCodeP, "unit code");
      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE7(nodeP->name, 'o', 'b', 'j', 'e', 'c', 't', 0))
    {
      DUPLICATE_CHECK(nodeP, objectP, "object");
      // FIXME: "object" for Property Attribute should be added as metadata
    }
    else if (SCOMPARE11(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
    {
      DUPLICATE_CHECK(nodeP, observedAtP, "observed at");
      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE17(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, observationSpaceP, "observation space");
      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE15(nodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, operationSpaceP, "operation space");
      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else  // Other
    {
      if (caP->metadataVector.lookupByName(nodeP->name) != NULL)
      {
        LM_E(("Duplicated attribute property '%s' for attribute '%s'", nodeP->name, caP->name.c_str()));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Duplicated attribute property", nodeP->name, OrionldDetailsString);
        return false;
      }

      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }

    nodeP = nodeP->next;
    LM_TMP(("Next Node: %p", nodeP));
  }

  LM_TMP(("After loop"));

  //
  // Mandatory fields for Property:
  //   type
  //   value (cannot be NULL)
  //
  // Mandatory fields for Relationship:
  //   type
  //   object (must be a JSON String)
  //
  if (typeP == NULL)  // Attr Type is mandatory!
  {
    LM_E(("'type' missing for attribute '%s'", kNodeP->name));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "attribute without 'type' field", kNodeP->name, OrionldDetailsAttribute);
    return false;
  }

  //
  // Setting the value of the property attribute
  //
  if (isProperty == true)
  {
    if (valueP == NULL)
    {
      if (isGeoProperty == true)
      {
        LM_E(("'value' missing for GeoProperty '%s'", kNodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "geo-property attribute without 'value' field", kNodeP->name, OrionldDetailsAttribute);
      }
      else if (isTemporalProperty == true)
      {
        LM_E(("'value' missing for TemporalProperty '%s'", kNodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "temporal-property attribute without 'value' field", kNodeP->name, OrionldDetailsAttribute);
      }
      else
      {
        LM_E(("'value' missing for Property '%s'", kNodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "property attribute without 'value' field", kNodeP->name, OrionldDetailsAttribute);
      }

      return false;
    }

    if (valueP->type == KjNull)
    {
      LM_E(("NULL 'value' for Property '%s'", kNodeP->name));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "property attribute with NULL 'value' field", kNodeP->name, OrionldDetailsAttribute);
      return NULL;
    }

    //
    // Get the value, and check for correct value for Geo/Temporal Attributes
    //
    if (isGeoProperty == true)
    {
      char* details = (char*) "no details";

      if (valueP->type != KjObject)
        ATTRIBUTE_ERROR("geo-property attribute value must be a JSON Object", kjValueType(valueP->type));
      else if (geojsonCheck(ciP, valueP, &details) == false)
      {
        LM_TMP(("geojsonCheck error for %s: %s", caName, details));
        ATTRIBUTE_ERROR("geo-property attribute must have a valid GeoJson value", details);
      }

      caP->valueType       = orion::ValueTypeObject;
      caP->compoundValueP  = compoundCreate(ciP, valueP, NULL, 0);
    }
    else if (isTemporalProperty == true)
    {
      if (valueP->type != KjString)
        ATTRIBUTE_ERROR("temporal-property attribute must have a value of type JSON String", kjValueType(valueP->type));
      else if (parse8601Time(valueP->value.s) == -1)
        ATTRIBUTE_ERROR("temporal-property attribute must have a valid ISO8601 as value", NULL);

      caP->valueType   = orion::ValueTypeString;
      caP->stringValue = valueP->value.s;

      // FIXME: Change the value type from string to Number, for easier check with filters?
    }
    else
    {
      //
      // "Normal" Property, can be any JSON type
      //
      switch (valueP->type)
      {
      case KjBoolean:    caP->valueType = orion::ValueTypeBoolean; caP->boolValue      = valueP->value.b; break;
      case KjInt:        caP->valueType = orion::ValueTypeNumber;  caP->numberValue    = valueP->value.i; break;
      case KjFloat:      caP->valueType = orion::ValueTypeNumber;  caP->numberValue    = valueP->value.f; break;
      case KjString:     caP->valueType = orion::ValueTypeString;  caP->stringValue    = valueP->value.s; break;
      case KjObject:     caP->valueType = orion::ValueTypeObject;  caP->compoundValueP = compoundCreate(ciP, valueP, NULL); break;
      case KjArray:      caP->valueType = orion::ValueTypeObject;  caP->compoundValueP = compoundCreate(ciP, valueP, NULL);  break;
      case KjNull:       caP->valueType = orion::ValueTypeNull;    break;
      case KjNone:
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal error", "Invalid type from kjson", OrionldDetailsString);
        return false;
      }
    }
  }
  else if (isRelationship == true)
  {
    char* details;

    if (objectP == NULL)
      ATTRIBUTE_ERROR("relationship attribute without 'object' field", caName);
    if (objectP->type != KjString)
      ATTRIBUTE_ERROR("relationship attribute with 'object' field of non-string type", caName);
    if ((urlCheck(objectP->value.s, &details) == false) && (urnCheck(objectP->value.s, &details) == false))
      ATTRIBUTE_ERROR("relationship attribute with 'object' field having invalid URI", objectP->value.s);

    caP->valueType   = orion::ValueTypeString;
    caP->stringValue = objectP->value.s;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// contextTreat -
//
static bool contextTreat
(
  ConnectionInfo*  ciP,
  KjNode*          contextNodeP,
  ContextElement*  ceP,
  char*            entityId
)
{
  LM_TMP(("contextNodeP at %p", contextNodeP));
  //
  // The allowed payloads for the @context member are:
  //
  // 1. ARRAY - An array of URL strings:
  //    "@context": [
  //      "http://...",
  //      "http://...",
  //      "http://..."
  //    }
  //
  // 2. STRING - A single URL string:
  //    "@context": "http://..."
  //
  // 3. OBJECT - Direct context:
  //    "@context": {
  //      "Property": "http://...",
  //      "XXX";      "YYY"
  //    }
  //
  // -----------------------------------------------------------
  // Case 3 is not implemented in the first round. coming later.
  // -----------------------------------------------------------
  //
  // As the payload is already parsed, what needs to be done here is to call orionldContextAdd() for each of these URLs
  //
  // The content of these "Context URLs" can be:
  //
  // 4. An object with a single member '@context' that is an object containing key-values:
  //    "@context" {
  //      "Property": "http://...",
  //      "XXX";      ""
  //    }
  //
  // 5. An object with a single member '@context', that is a vector of URL strings (https://fiware.github.io/NGSI-LD_Tests/ldContext/testFullContext.jsonld):
  //    {
  //      "@context": [
  //        "http://...",
  //        "http://...",
  //        "http://..."
  //      }
  //    }
  //
  LM_T(LmtContextTreat, ("Got @context for '%s', type %s", entityId, kjValueType(contextNodeP->type)));

  if (contextNodeP->type == KjString)
  {
    char* details;

    LM_TMP(("Calling orionldContextCreateFromUrl"));
    if ((ciP->contextP = orionldContextCreateFromUrl(ciP, contextNodeP->value.s, OrionldUserContext, &details)) == NULL)
    {
      LM_E(("Failed to create context from URL: %s", details));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from URL", details, OrionldDetailsString);
      return false;
    }
    LM_TMP(("orionldContextCreateFromUrl OK"));
    ciP->contextToBeFreed = false;  // context has been added to public list - must not be freed
  }
  else if (contextNodeP->type == KjArray)
  {
    char* details;

    //
    // REMEMBER
    //   This context is just the array of context-strings: [ "url1", "url2" ]
    //   The individual contexts ("url1", "url2") are treated a few lines down
    //
    // contextUrl = "http://" host + ":" + port + "/ngsi-ld/ex/v1/contexts/" + entity.id;
    //
    int   linkPathLen = 7 + orionldHostNameLen + 1 + 5 + 27 + strlen(entityId) + 1;
    char* linkPath    = (char*) malloc(linkPathLen);

    if (linkPath == NULL)
    {
      LM_E(("out of memory creating Link HTTP Header"));
      orionldErrorResponseCreate(ciP, OrionldInternalError, "cannot create Link HTTP Header", "out of memory", OrionldDetailsString);
      return false;
    }
    sprintf(linkPath, "http://%s:%d/ngsi-ld/ex/v1/contexts/%s", orionldHostName, restPortGet(), entityId);

    ciP->contextP = orionldContextCreateFromTree(contextNodeP, linkPath, OrionldUserContext, &details);
    free(linkPath);  // orionldContextCreateFromTree strdups the URL

    if (ciP->contextP == NULL)
    {
      LM_E(("Failed to create context from Tree : %s", details));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from tree", details, OrionldDetailsString);
      return false;
    }

    //
    // The context containing a vector of X context strings (URLs) must be freed
    // The downloaded contexts though are added to the public list and will not be freed)
    //
    ciP->contextToBeFreed = true;

    for (KjNode* contextStringNodeP = contextNodeP->children; contextStringNodeP != NULL; contextStringNodeP = contextStringNodeP->next)
    {
      if (contextStringNodeP->type != KjString)
      {
        LM_E(("Context Array Item is not a JSON String, but of type '%s'", kjValueType(contextStringNodeP->type)));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Context Array Item is not a JSON String", NULL, OrionldDetailsString);
        return false;
      }

      if (contextItemNodeTreat(ciP, contextStringNodeP->value.s) == NULL)
      {
        LM_E(("contextItemNodeTreat failed"));
        // Error payload set by contextItemNodeTreat
        return false;
      }
    }
  }
  else if (contextNodeP->type == KjObject)
  {
    // FIXME: seems like an inline context - not supported for now
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", "inline contexts not supported in current version of orionld", OrionldDetailsString);
    LM_E(("inline contexts not supported in current version of orionld"));
    return false;
  }
  else
  {
    LM_E(("invalid JSON type of @context member"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", "invalid JSON type of @context member", OrionldDetailsString);
    return false;
  }


#if 0
  //
  // Check the validity of the context
  //

  //
  // FIXME: task/28.orionld-user-context-cannot-use-core-context-values
  //        The check is implemented, but ... six different types of contexts ... just too much
  //        This test (orionldUserContextKeyValuesCheck) is postponed until hardening/xx.orionld-order-in-contexts
  //        is implemented.
  //
  char* details;

  LM_TMP(("orionldPostEntities calling orionldUserContextKeyValuesCheck"));
  if (orionldUserContextKeyValuesCheck(ciP->contextP->tree, ciP->contextP->url, &details) == false)
  {
    LM_TMP(("orionldPostEntities called orionldUserContextKeyValuesCheck(%s): %s", ciP->contextP->url, details));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", details, OrionldDetailsString);
    return false;
  }
  LM_TMP(("orionldPostEntities called orionldUserContextKeyValuesCheck: %s", "OK"));
#endif

  LM_T(LmtContextTreat, ("The @context is treated as an attribute"));
  // The @context is treated as an attribute
  ContextAttribute* caP;

  // The attribute's value is either a string or a vector (compound)
  if (contextNodeP->type == KjString)
  {
    caP = new ContextAttribute("@context", "ContextString", contextNodeP->value.s);
  }
  else
  {
    // Create the Compound, just a vector of strings
    orion::CompoundValueNode* compoundP = new orion::CompoundValueNode(orion::ValueTypeVector);
    int                       siblingNo = 0;

    // Loop over the kNode vector and create the strings
    for (KjNode* contextItemNodeP = contextNodeP->children; contextItemNodeP != NULL; contextItemNodeP = contextItemNodeP->next)
    {
      LM_T(LmtContextTreat, ("string: %s", contextItemNodeP->value.s));
      orion::CompoundValueNode* stringNode = new orion::CompoundValueNode(compoundP, "", "", contextItemNodeP->value.s, siblingNo++, orion::ValueTypeString);

      compoundP->add(stringNode);
    }

    // Now set 'compoundP' as value of the attribute
    caP = new ContextAttribute();

    caP->type           = "ContextVector";
    caP->name           = "@context";
    caP->valueType      = orion::ValueTypeObject;  // All compounds have Object as value type (I think)
    caP->compoundValueP = compoundP;
  }

  ceP->contextAttributeVector.push_back(caP);

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPostEntities -
//
bool orionldPostEntities(ConnectionInfo* ciP)
{
  char*    details;
  KjNode*  idNodeP            = NULL;
  KjNode*  typeNodeP          = NULL;
  KjNode*  locationP          = NULL;
  KjNode*  contextNodeP       = NULL;
  KjNode*  observationSpaceP  = NULL;
  KjNode*  operationSpaceP    = NULL;
  KjNode*  kNodeP;

  if (payloadCheck(ciP, &idNodeP, &typeNodeP, &locationP, &contextNodeP, &observationSpaceP, &operationSpaceP) == false)
    return false;

  LM_T(LmtUriExpansion, ("type node at %p", typeNodeP));
  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  mongoRequest.contextElementVector.push_back(ceP);
  entityIdP = &mongoRequest.contextElementVector[0]->entityId;
  mongoRequest.updateActionType = ActionTypeAppend;

  //
  // First treat the @context, if none, use the default context
  // contextTreat needs cpP to push the '@context' attribute to the ContextElement.
  //
  if ((contextNodeP != NULL) && (contextTreat(ciP, contextNodeP, ceP, idNodeP->value.s) == false))
  {
    // Error payload set by contextTreat
    mongoRequest.release();
    return false;
  }

  // Treat the entire payload
  for (kNodeP = ciP->requestTree->children; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    LM_T(LmtUriExpansion, ("treating entity node '%s'", kNodeP->name));

    // Entity ID
    if (kNodeP == idNodeP)
    {
      if (!urlCheck(idNodeP->value.s, &details) && !urnCheck(idNodeP->value.s, &details))
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Entity ID", "Not a URL nor a URN", OrionldDetailsString);
        mongoRequest.release();
        return false;
      }

      //
      // If the entity already exists, an error should be returned
      //
      if (mongoEntityExists(idNodeP->value.s, ciP->tenant.c_str()) == true)
      {
        orionldErrorResponseCreate(ciP, OrionldAlreadyExists, "Entity already exists", idNodeP->value.s, OrionldDetailsString);
        ciP->httpStatusCode = SccConflict;
        mongoRequest.release();
        return false;
      }

      entityIdP->id        = idNodeP->value.s;
      entityIdP->type      = (typeNodeP != NULL)? typeNodeP->value.s : NULL;
      entityIdP->isPattern = false;
      entityIdP->creDate   = getCurrentTime();
      entityIdP->modDate   = getCurrentTime();

      continue;
    }

    // Entity TYPE
    else if (kNodeP == typeNodeP)
    {
      entityIdP->isTypePattern = false;

      LM_T(LmtUriExpansion, ("Looking up uri expansion for the entity type '%s'", typeNodeP->value.s));
      LM_T(LmtUriExpansion, ("------------- uriExpansion for Entity Type starts here ------------------------------"));
      char*  expandedName;
      char*  expandedType;
      int    expansions = uriExpansion(ciP->contextP, typeNodeP->value.s, &expandedName, &expandedType, &details);
      LM_T(LmtUriExpansion, ("Got %d expansions", expansions));

      if (expansions == -1)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context item for 'entity type'", details, OrionldDetailsString);
        mongoRequest.release();
        return false;
      }
      else if (expansions == 0)
      {
        // Expansion found in Core Context - perform no expansion
      }
      else if (expansions == -2)
      {
        // No expansion found in Core Context, and in no other contexts either - use default URL
        LM_TMP(("EXPANSION: use default URL for entity type '%s'", typeNodeP->value.s));
        entityIdP->type = orionldDefaultUrl;
        entityIdP->type += typeNodeP->value.s;
      }
      else if (expansions == 1)
      {
        // Take the long name, just ... NOT expandedType but expandedName. All good
        entityIdP->type = expandedName;
      }
      else  // expansions == 2 ... may be an incorrect context
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value of context item 'entity id'", ciP->contextP->url, OrionldDetailsString);
        mongoRequest.release();
        return false;
      }
      continue;
    }
    else if (kNodeP == contextNodeP)
    {
      // All is done already
      continue;
    }
    else  // Must be an attribute
    {
      LM_T(LmtPayloadCheck, ("Not id/type/@context: '%s' - treating as attribute", kNodeP->name));

      ContextAttribute* caP            = new ContextAttribute();
      KjNode*           attrTypeNodeP  = NULL;

      if (attributeTreat(ciP, kNodeP, caP, &attrTypeNodeP) == false)
      {
        ciP->httpStatusCode = SccBadRequest;  // FIXME: Should be set inside 'attributeTreat' - could be 500, not 400 ...
        LM_TMP(("attributeTreat failed"));
        delete caP;
        mongoRequest.release();
        return false;
      }


      //
      // URI Expansion for Attribute NAME - except if its name is one of the following three:
      // 1. location
      // 2. observationSpace
      // 3. operationSpace
      //
      if ((strcmp(kNodeP->name, "location") == 0) || (strcmp(kNodeP->name, "observationSpace") == 0) || (strcmp(kNodeP->name, "operationSpace") == 0))
      {
        caP->name = kNodeP->name;
      }
      else
      {
        LM_T(LmtUriExpansion, ("------------- URI-Expansion for attribute named '%s' starts here ------------------------------", kNodeP->name));
        char*  details;
        char*  expandedName  = NULL;
        char*  expandedType  = NULL;
        int    expansions    = uriExpansion(ciP->contextP, kNodeP->name, &expandedName, &expandedType, &details);

        LM_TMP(("EXPANSION: uriExpansion returned %d for '%s'", expansions, kNodeP->name));
        if (expansions == -1)
        {
          delete caP;
          mongoRequest.release();
          orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context item for 'attribute name'", kNodeP->name, OrionldDetailsAttribute);
          return false;
        }
        else if (expansions == -2)
        {
          // No expansion found in Core Context, and in no other contexts either - use default URL
          LM_TMP(("EXPANSION: use default URL for attribute '%s'", kNodeP->name));
          caP->name     = orionldDefaultUrl;
          caP->name    += kNodeP->name;
        }
        else if (expansions == 0)
        {
          // Found in Core Context - no expansion
          caP->name = kNodeP->name;
        }
        else
        {
          // Change the attribute name to its expanded value
          if (expandedName == NULL)
          {
            LM_T(LmtUriExpansion, ("No expansion found - perform default expansion - prepending http://www.example.org/attribute/ to the attr name (%s)", kNodeP->name));
            caP->name  = orionldDefaultUrl;
            caP->name += kNodeP->name;
          }
          else
            caP->name = expandedName;
        }
      }

      //
      // NO URI Expansion for Attribute TYPE
      //
      caP->type = attrTypeNodeP->value.s;

      ceP->contextAttributeVector.push_back(caP);
    }
  }

  LM_TMP(("Calling mongoUpdateContext"));
  ciP->httpStatusCode = mongoUpdateContext(&mongoRequest,
                                           &mongoResponse,
                                           ciP->httpHeaders.tenant,
                                           ciP->servicePathV,
                                           ciP->uriParam,
                                           ciP->httpHeaders.xauthToken,
                                           ciP->httpHeaders.correlator,
                                           ciP->httpHeaders.ngsiv2AttrsFormat,
                                           ciP->apiVersion,
                                           NGSIV2_NO_FLAVOUR);
  LM_TMP(("mongoUpdateContext returned %d", ciP->httpStatusCode));
  mongoRequest.release();
  mongoResponse.release();

  if (ciP->httpStatusCode != SccOk)
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", ciP->httpStatusCode));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal Error", "Error from mongo backend", OrionldDetailsString);
    return false;
  }

  ciP->httpStatusCode = SccCreated;
  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/entities/", idNodeP->value.s);
  httpHeaderLinkAdd(ciP, (ciP->contextP == NULL)? ORIONLD_CORE_CONTEXT_URL : ciP->contextP->url);

  LM_TMP(("Function Done"));
  return true;
}
