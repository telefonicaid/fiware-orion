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

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionTypes/OrionValueType.h"                         // orion::ValueType
#include "orionTypes/UpdateActionType.h"                       // ActionType
#include "parse/CompoundValueNode.h"                           // CompoundValueNode
#include "ngsi/ContextAttribute.h"                             // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                       // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                      // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                   // mongoUpdateContext

#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/context/orionldDefaultContext.h"             // orionldDefaultContext
#include "orionld/context/orionldContextAdd.h"                 // Add a context to the context list
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextItemLookup.h"          // orionldContextItemLookup
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldContextList.h"                // orionldContextHead, orionldContextTail
#include "orionld/serviceRoutines/orionldPostEntities.h"       // Own interface



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
    LM_T(LmtPayloadCheck, ("the node is a '%s'", kjValueType(nodeP->type)));                      \
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
  LM_T(LmtContextTreat, ("In contextItemNodeTreat. url == '%s'", url));

  char*            details;
  OrionldContext*  contextP = orionldContextAdd(ciP, url, &details);

  if (contextP == NULL)
  {
    LM_E(("Invalid context '%s': %s", url, details));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", details);
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
    *detailsPP = (char*) "invalid name";
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
  KjNode*  locationNodeP          = NULL;
  KjNode*  contextNodeP           = NULL;
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
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid entity type name", detailsP);
        return false;
      }
    }
    else if (SCOMPARE9(kNodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(kNodeP, locationNodeP, "location");
      // FIXME: check validity of location
    }
    else if (SCOMPARE9(kNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      DUPLICATE_CHECK(kNodeP, contextNodeP, "context");
      ARRAY_OR_STRING_CHECK(kNodeP, "@context");
    }
    else if (SCOMPARE17(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(kNodeP, observationSpaceNodeP, "context");
      // FIXME: check validity of observationSpace
    }
    else if (SCOMPARE15(kNodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(kNodeP, operationSpaceNodeP, "context");
      // FIXME: check validity of operationSpaceP
    }
    else  // Property/Relationshiop - must check chars in the name of the attribute
    {
      // FIXME: Make sure the type is either Property or Relationship
      if (stringContentCheck(kNodeP->name, &detailsP) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Property/Relationship name", detailsP);
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
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "No 'id' of the entity", "The 'id' field is mandatory");
    return false;
  }

  if (typeNodeP == NULL)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "No 'type' of the entity", "The 'type' field is mandatory");
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
// Returns the number of expansions done.
//  -1: an error has occurred
//   0: no expansions found
//   1: expansion found only for the attr-name/entity-type
//   2: expansions found both for attr-name and attr-type
//
static int uriExpansion(OrionldContext* contextP, const char* name, char** expandedNameP, char** expandedTypeP, char** detailsPP)
{
  *expandedNameP = NULL;
  *expandedTypeP = NULL;

  LM_T(LmtUriExpansion, ("looking up context item for '%s', in context '%s'", name, contextP->url));
  KjNode* contextValueP = orionldContextItemLookup(contextP, name);
  LM_T(LmtUriExpansion, ("contextValueP at %p", contextValueP));
    
  if (contextValueP == NULL)
  {
    LM_T(LmtUriExpansion, ("no context found"));
    return 0;
  }

  if (contextValueP->type == KjString)
  {
    LM_T(LmtUriExpansion, ("got a string - expanded name is '%s'", contextValueP->value.s));
    *expandedNameP = contextValueP->value.s;

    return 1;
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

#if 1
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
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context item", details);
      return NULL;
    }

    if (expandedNameP != NULL)
    {
      LM_T(LmtUriExpansion, ("Expansion found for %s name '%s': %s", kjValueType(kNodeP->type), kNodeP->name, expandedNameP));
      cNodeP->name = expandedNameP;
    }
    else
    {
      LM_T(LmtUriExpansion, ("NO Expansion found for %s name '%s': using: %s%s", kjValueType(kNodeP->type), kNodeP->name, ORIONLD_DEFAULT_EXPANSION_URL_DIR_ATTRIBUTE, kNodeP->name));
      cNodeP->name = std::string(ORIONLD_DEFAULT_EXPANSION_URL_DIR_ATTRIBUTE) + kNodeP->name;
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
      // Skip 'type' if in level 1
      if ((level == 1) && (kChildP->type == KjString) && (SCOMPARE5(kChildP->name, 't', 'y', 'p', 'e', 0)))
      {
        LM_T(LmtCompoundCreation, ("Skipping '%s' node on level %d", kChildP->name, level));
        continue;
      }

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
// attributeTreat - NO
//
// We will need more than one function;
// - propertyTreat
// - geoPropertyTreat ?
// - relationshipTreat
//
static bool attributeTreat(ConnectionInfo* ciP, KjNode* kNodeP, ContextAttribute* caP, KjNode** typeNodePP)
{
  LM_T(LmtPayloadCheck, ("Treating attribute '%s' (KjNode at %p)", kNodeP->name, kNodeP));

  OBJECT_CHECK(kNodeP, "attribute");

  KjNode* typeP              = NULL;  // For ALL:            Mandatory
  KjNode* valueP             = NULL;  // For 'Property':     Mandatory
  KjNode* unitCodeP          = NULL;  // For 'Property':     Optional
  KjNode* objectP            = NULL;  // For 'Relationship:  Mandatory
  KjNode* observedAtP        = NULL;  // For ALL:            Optional
  KjNode* observationSpaceP  = NULL;  // For 'GeoProperty':  Optional
  KjNode* operationSpaceP    = NULL;  // For 'GeoProperty':  Optional
  KjNode* nodeP              = kNodeP->children;

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
  bool isProperty     = false;
  bool isGeoProperty  = false;
  bool isRelationship = false;

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
      else
      {
        LM_E(("Invalid type for attribute '%s': '%s'", nodeP->name, nodeP->value.s));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid type for attribute", nodeP->value.s);
        return false;
      }

      *typeNodePP = typeP;
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
    else if (SCOMPARE11(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
    {
      DUPLICATE_CHECK(nodeP, observedAtP, "observed at");
    }
    else if (SCOMPARE17(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, observationSpaceP, "observation space");
    }
    else if (SCOMPARE15(nodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(nodeP, operationSpaceP, "operation space");
    }
    else  // Other
    {
    }

    nodeP = nodeP->next;
  }


  //
  // Mandatory fields for Property:
  //   type
  //   value
  //   
  // Mandatory fields for Relationship:
  //   type
  //   object
  //  
  if (typeP == NULL)  // Attr Type is mandatory!
  {
    LM_E(("'type' missing for attribute '%s'", kNodeP->name));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "attribute without 'type' field", kNodeP->name);
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
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "geo-property attribute without 'value' field", kNodeP->name);
      }
      else
      {
        LM_E(("'value' missing for Property '%s'", kNodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "property attribute without 'value' field", kNodeP->name);
      }

      return false;
    }

    switch (kNodeP->type)
    {
    case KjBoolean:    caP->valueType = orion::ValueTypeBoolean; caP->boolValue      = kNodeP->value.b; break;
    case KjInt:        caP->valueType = orion::ValueTypeNumber;  caP->numberValue    = kNodeP->value.i; break;
    case KjFloat:      caP->valueType = orion::ValueTypeNumber;  caP->numberValue    = kNodeP->value.f; break;
    case KjString:     caP->valueType = orion::ValueTypeString;  caP->stringValue    = kNodeP->value.s; break;
    case KjObject:     caP->valueType = orion::ValueTypeObject;  caP->compoundValueP = compoundCreate(ciP, kNodeP, NULL); break;
    case KjArray:      caP->valueType = orion::ValueTypeObject;  caP->compoundValueP = compoundCreate(ciP, kNodeP, NULL);  break;
    case KjNull:       caP->valueType = orion::ValueTypeNull;    break;
    case KjNone:
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal error", "Invalid type from kjson");
      return false;
    }
  }
  else if (isRelationship == true)
  {
    if (objectP == NULL)
    {
      LM_E(("'object' missing for Relationship '%s'", kNodeP->name));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "relationship attribute without 'object' field", NULL);
      return false;
    }

    if (objectP->type != KjString)
    {
      LM_E(("Relationship '%s': 'object' is not a JSON String", objectP->name));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "relationship attribute with 'object' field of non-string type", NULL);
      return false;
    }

    char* details;
    if (urlCheck(objectP->value.s, &details) == false)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "relationship attribute with 'object' field having invalid URL", objectP->value.s);
      return false;
    }
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
  ContextElement*  ceP
)
{
  if (contextNodeP == NULL)
  {
    if (ciP->httpHeaders.link != "")
    {
      char* details;

      if (urlCheck((char*) ciP->httpHeaders.link.c_str(), &details) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Link HTTP Header must be a valid URL", details);
        return false;
      }

      if ((ciP->contextP = orionldContextCreateFromUrl(ciP, ciP->httpHeaders.link.c_str(), &details)) == NULL)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from URL", details);
        return false;
      }
    }
    else
    {
      LM_T(LmtUriExpansion, ("Setting the context to the default context"));
      ciP->contextP = &orionldDefaultContext;
    }

    return true;
  }
  
  //
  // In the first implementation of ngsi-ld, the allowed payloads for the @context member are:
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
  // case 3 is not implemented in the first round. coming later
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
  LM_T(LmtContextTreat, ("Got a @context, of type %s", kjValueType(contextNodeP->type)));

  if (contextNodeP->type == KjString)
  {
    char* details;

    if ((ciP->contextP = orionldContextCreateFromUrl(ciP, contextNodeP->value.s, &details)) == NULL)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from URL", details);
      return false;
    }

    orionldContextListInsert(ciP->contextP);
    ciP->contextToBeFreed = false;  // context has been added to public list - must not be freed
  }
  else if (contextNodeP->type == KjArray)
  {
    char* details;

    // FIXME: When orionld is able to serve contexts, a real URL must be used here
    ciP->contextP = orionldContextCreateFromTree(contextNodeP, "http;//FIXME.array.context.needs/url", &details);
    if (ciP->contextP == NULL)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from tree", details);
      return false;
    }

    //
    // The context containing a vector of X context strings (URLs) must be freed
    // The downloaded contexts though are added to the public list and will not be freed)
    //
    ciP->contextToBeFreed = true;

    LM_T(LmtContextTreat, ("The context is a VECTOR OF STRINGS"));
    for (KjNode* contextStringNodeP = contextNodeP->children; contextStringNodeP != NULL; contextStringNodeP = contextStringNodeP->next)
    {
      if (contextStringNodeP->type != KjString)
      {
        LM_E(("Context Array Item is not a JSON String, but of type '%s'", kjValueType(contextStringNodeP->type)));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Context Array Item is not a JSON String", NULL);
        LM_T(LmtContextTreat, ("returning FALSE"));
        return false;
      }

      if (contextItemNodeTreat(ciP, contextStringNodeP->value.s) == NULL)
      {
        LM_T(LmtContextTreat, ("contextItemNodeTreat failed"));
        // Error payload set by contextItemNodeTreat
        return false;
      }
    }
  }
  else if (contextNodeP->type == KjObject)
  {
    // FIXME: seems like an inline context - not supported for now
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", "inline contexts not supported in current version of orionld");
    LM_E(("inline contexts not supported in current version of orionld"));
    return false;
  }
  else
  {
    LM_E(("invalid JSON type of @context member"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", "invalid JSON type of @context member");
    return false;
  }

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
  ContextElement*        ceP       = new ContextElement();  // FIXME: Do I really need to allocate ?
  EntityId*              entityIdP;

  mongoRequest.contextElementVector.push_back(ceP);
  entityIdP = &mongoRequest.contextElementVector[0]->entityId;

  mongoRequest.updateActionType = ActionTypeAppendStrict;  // Error if entity already exists, I hope

  // First treat the @context, if none, use the default context
  if (contextTreat(ciP, contextNodeP, ceP) == false)
  {
    // Error payload set by contextTreat
    mongoRequest.release();
    return false;
  }
  

  // Treat the entire payload
  for (kNodeP = ciP->requestTree->children; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    LM_T(LmtUriExpansion, ("treating entity node '%s'", kNodeP->name));

    if (kNodeP == idNodeP)
    {
      if (!urlCheck(idNodeP->value.s, &details) && !urnCheck(idNodeP->value.s, &details))
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid Entity ID", "Not a URL nor a URN");
        mongoRequest.release();
        return false;
      }

      entityIdP->id        = idNodeP->value.s;
      entityIdP->isPattern = false;
      entityIdP->creDate   = getCurrentTime();
      entityIdP->modDate   = getCurrentTime();

      // Lookup entityIdP->id in @context?
      continue;
    }
    else if (kNodeP == typeNodeP)
    {
      entityIdP->isTypePattern = false;

      LM_T(LmtUriExpansion, ("Looking up uri expansion for the entity type '%s'", typeNodeP->value.s));
      LM_T(LmtUriExpansion, ("------------- uriExpansion for Entity Type starts here ------------------------------"));
      char*  expandedName;
      char*  expandedType;
      int    expansions = uriExpansion(ciP->contextP, typeNodeP->value.s, &expandedName, &expandedType, &details);

      if (expansions == -1)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context item for 'entity type'", details);
        mongoRequest.release();
        return false;
      }
      else if (expansions == 0)
      {
        // No expansion found - perform default expansion
        entityIdP->type = std::string(ORIONLD_DEFAULT_EXPANSION_URL_DIR_ENTITY) + typeNodeP->value.s;
      }
      else if (expansions == 1)
      {
        // Take the long name, just ... NOT expandedType but expandedName. All Good
        entityIdP->type = expandedName;

        // Add '@aliasEntityType' as an attribute, with original entity type as value, and change the entity type to 'expansion'
        ContextAttribute* caP = new ContextAttribute("@aliasEntityType", "Alias", typeNodeP->value.s);
        ceP->contextAttributeVector.push_back(caP);
      }      
      else  // expansions == 2 ... may be an incorrect context
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value of context item 'entity id'", ciP->contextP->url);
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
        delete caP;
        mongoRequest.release();
        return false;
      }


      //
      // URI Expansion for Attribute NAME
      //
      LM_T(LmtUriExpansion, ("Calling uriExpansion for the attribute name '%s'", kNodeP->name));
      LM_T(LmtUriExpansion, ("------------- uriExpansion for attr name starts here ------------------------------"));
      char*  expandedName = NULL;
      char*  expandedType = NULL;
      char*  details;
      int    expansions   = uriExpansion(ciP->contextP, kNodeP->name, &expandedName, &expandedType, &details);

      if (expansions == -1)
      {
        delete caP;
        mongoRequest.release();
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context item for 'attribute name'", details);
        return false;
      }
      else if (expansions >= 0)
      {
        // Add '@aliasAttributeName' as a metadata, with original attr name as value, and change the 'real' attr name to 'expansion'
        Metadata* mdP = new Metadata("@aliasAttributeName", "Alias", kNodeP->name);
        caP->metadataVector.push_back(mdP);

        // Now change the attribute name to its expanded value
        if (expandedName == NULL)
        {
          LM_T(LmtUriExpansion, ("No expansion found - perform default expansion - prepending http://www.example.org/attribute/ to the attr name (%s)", kNodeP->name));
          caP->name = std::string(ORIONLD_DEFAULT_EXPANSION_URL_DIR_ATTRIBUTE) + kNodeP->name;
        }
        else
        {
          caP->name = expandedName;
        }
      }


      
      //
      // URI Expansion for Attribute TYPE
      //
      LM_T(LmtUriExpansion, ("Calling uriExpansion for the attribute type '%s'", attrTypeNodeP->value.s));
      LM_T(LmtUriExpansion, ("------------- uriExpansion for attr type starts here ------------------------------"));

      expandedName = NULL;
      expandedType = NULL;
      expansions   = uriExpansion(ciP->contextP, attrTypeNodeP->value.s, &expandedName, &expandedType, &details);

      if (expansions == -1)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context item for 'attribute type'", details);
        delete caP;
        mongoRequest.release();
        return false;
      }
      else if (expansions >= 0)
      {
        // Add '@aliasAttributeType' as a metadata, with original attr type as value, and change the 'real' attr type to 'expansion'
        Metadata* mdP = new Metadata("@aliasAttributeType", "Alias", attrTypeNodeP->value.s);
        caP->metadataVector.push_back(mdP);

        // Now change the attribute type to its expanded value
        if (expandedName == NULL)
        {
          // No expansion found - perform default expansion
          LM_T(LmtUriExpansion, ("No expansion found - perform default expansion - prepending http://www.example.org/attribute/ to the attr type (%s)", attrTypeNodeP->value.s));
          caP->type = std::string(ORIONLD_DEFAULT_EXPANSION_URL_DIR_ATTRIBUTE) + attrTypeNodeP->value.s;
        }
        else
        {
          caP->type = expandedName;
        }
      }

      ceP->contextAttributeVector.push_back(caP);
    }
  }

  // ngsi-ld doesn't use service path - so always '/'
  ciP->servicePathV.push_back("/");

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
  mongoRequest.release();
  mongoResponse.release();

  if (ciP->httpStatusCode != SccOk)
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", ciP->httpStatusCode));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal Error", "Error from mongo backend");
    return false;
  }

  ciP->httpStatusCode = SccCreated;
  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/entities/", idNodeP->value.s);
  httpHeaderLinkAdd(ciP, (ciP->contextP == NULL)? ORIONLD_DEFAULT_CONTEXT_URL : ciP->contextP->url);

  return true;
}
