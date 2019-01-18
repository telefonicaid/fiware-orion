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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString, kjObject, ...
#include "kjson/kjRender.h"                                    // kjRender
}

#include "common/globals.h"                                    // parse8601Time
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderLocationAdd, httpHeaderLinkAdd
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
#include "orionld/common/CHECK.h"                              // CHECK
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/common/geoJsonCheck.h"                       // geoJsonCheck
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextAdd.h"                 // Add a context to the context list
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextItemLookup.h"          // orionldContextItemLookup
#include "orionld/context/orionldContextList.h"                // orionldContextHead, orionldContextTail
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldUserContextKeyValuesCheck.h"  // orionldUserContextKeyValuesCheck
#include "orionld/context/orionldContextTreat.h"               // orionldContextTreat
#include "orionld/context/orionldUriExpand.h"                  // orionldUriExpand
#include "orionld/serviceRoutines/orionldPostEntities.h"       // Own interface



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

  KjNode*  kNodeP                 = ciP->requestTree->value.firstChildP;
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
      DUPLICATE_CHECK(idNodeP, "entity id", kNodeP);
      STRING_CHECK(kNodeP, "entity id");
    }
    else if (SCOMPARE5(kNodeP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(typeNodeP, "entity type", kNodeP);
      STRING_CHECK(kNodeP, "entity type");
      if (stringContentCheck(kNodeP->value.s, &detailsP) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid entity type name", detailsP, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE9(kNodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(locationNodeP, "location", kNodeP);
      // FIXME: check validity of location - GeoProperty
    }
    else if (SCOMPARE9(kNodeP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      DUPLICATE_CHECK(contextNodeP, "context", kNodeP);
      ARRAY_OR_STRING_CHECK(kNodeP, "@context");
    }
    else if (SCOMPARE17(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(observationSpaceNodeP, "context", kNodeP);
      // FIXME: check validity of observationSpace - GeoProperty
    }
    else if (SCOMPARE15(kNodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(operationSpaceNodeP, "context", kNodeP);
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

    for (KjNode* kChildP = kNodeP->value.firstChildP; kChildP != NULL; kChildP = kChildP->next)
    {
      orion::CompoundValueNode* cChildP = compoundCreate(ciP, kChildP, kNodeP, level);
      cNodeP->childV.push_back(cChildP);
    }
  }
  else if (kNodeP->type == KjArray)
  {
    ++level;
    cNodeP->valueType = orion::ValueTypeVector;

    for (KjNode* kChildP = kNodeP->value.firstChildP; kChildP != NULL; kChildP = kChildP->next)
    {
      orion::CompoundValueNode* cChildP = compoundCreate(ciP, kChildP, kNodeP, level);

      cNodeP->childV.push_back(cChildP);
    }
  }

  return cNodeP;
}



// -----------------------------------------------------------------------------
//
// metadataAdd -
//
static bool metadataAdd(ConnectionInfo* ciP, ContextAttribute* caP, KjNode* nodeP, char* caName)
{
  LM_T(LmtMetadata, ("Create metadata '%s' (a JSON %s) and add to attribute '%s'", nodeP->name, kjValueType(nodeP->type), caName));

  KjNode*   typeNodeP       = NULL;
  KjNode*   valueNodeP      = NULL;
  KjNode*   objectNodeP     = NULL;
  bool      isProperty      = false;
  bool      isRelationship  = false;

  if (nodeP->type == KjObject)
  {
    for (KjNode* kNodeP = nodeP->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
    {
      if (SCOMPARE5(kNodeP->name, 't', 'y', 'p', 'e', 0))
      {
        DUPLICATE_CHECK(typeNodeP, "metadata type", kNodeP);
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
        DUPLICATE_CHECK(valueNodeP, "metadata value", kNodeP);
        valueNodeP = kNodeP;
      }
      else if (SCOMPARE7(kNodeP->name, 'o', 'b', 'j', 'e', 'c', 't', 0))
      {
        DUPLICATE_CHECK(objectNodeP, "metadata object", kNodeP);
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
// specialCompoundCheck - 
//
bool specialCompoundCheck(ConnectionInfo* ciP, KjNode* compoundValueP)
{
  KjNode*  typeNodeP  = NULL;
  KjNode*  valueNodeP = NULL;
  KjNode*  otherNodeP = NULL;

  for (KjNode* nodeP = compoundValueP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (nodeP->name[0] == '@')
    {
      if (SCOMPARE6(nodeP->name, '@', 't', 'y', 'p', 'e', 0))
      {
        DUPLICATE_CHECK(typeNodeP, "@type", nodeP);
        STRING_CHECK(nodeP, "@type");
      }
      else if (SCOMPARE7(nodeP->name, '@', 'v', 'a', 'l', 'u', 'e', 0))
      {
        DUPLICATE_CHECK(valueNodeP, "@value", nodeP);
      }
      else
        otherNodeP = nodeP;
    }
    else
      otherNodeP = nodeP;
  }

  if ((typeNodeP != NULL) && (valueNodeP != NULL))
  {
    if (otherNodeP != NULL)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "unwanted extra items in @value/@type compound", otherNodeP->name, OrionldDetailsString);
      return false;
    }

    // All info needed to check validity of @value
    if (SCOMPARE9(typeNodeP->value.s, 'D', 'a', 't', 'e', 'T', 'i', 'm', 'e', 0))
    {
      STRING_CHECK(valueNodeP, "@value of DateTime @type");

      if (parse8601Time(valueNodeP->value.s) == -1)
      {
        ATTRIBUTE_ERROR("DateTime value of @value/@type compound must be a valid ISO8601", NULL);
      }
    }
  }
  else if (valueNodeP != NULL)
  {
    if (otherNodeP != NULL)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "unwanted extra items in @value/@type compound", otherNodeP->name, OrionldDetailsString);
      return false;
    }
  }
  else if (typeNodeP != NULL)
  {
    if (valueNodeP == NULL)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "missing @value in @value/@type compound", "@value is mandatory", OrionldDetailsString);
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// attributeTreat -
//
// We will need more than one function;
// - propertyTreat
// - geoPropertyTreat ?
// - relationshipTreat
//
// FIXME: Move to separate file attributeTreat.[cpp/h]
//
bool attributeTreat(ConnectionInfo* ciP, KjNode* kNodeP, ContextAttribute* caP, KjNode** typeNodePP)
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
  KjNode* nodeP              = kNodeP->value.firstChildP;

  while (nodeP != NULL)
  {
    LM_T(LmtPayloadCheck, ("Treating part '%s' of attribute '%s'", nodeP->name, kNodeP->name));

    if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
    {
      DUPLICATE_CHECK(typeP, "attribute type", nodeP);
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
        isTemporalProperty = true;
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
      DUPLICATE_CHECK(valueP, "attribute value", nodeP);
      // FIXME: "value" for Relationship Attribute should be added as metadata
   }
    else if (SCOMPARE9(nodeP->name, 'u', 'n', 'i', 't', 'C', 'o', 'd', 'e', 0))
    {
      DUPLICATE_CHECK(unitCodeP, "unit code", nodeP);
      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE7(nodeP->name, 'o', 'b', 'j', 'e', 'c', 't', 0))
    {
      DUPLICATE_CHECK(objectP, "object", nodeP);
      // FIXME: "object" for Property Attribute should be added as metadata
    }
    else if (SCOMPARE11(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
    {
      DUPLICATE_CHECK(observedAtP, "observed at", nodeP);
      STRING_CHECK(nodeP, "observed at");

      //
      // This is a very special attribute.
      // It's value must be a JSON String and the string must be a valid ISO8601 dateTime
      // The string is changed for a Number before stored in the database
      //
      int64_t dateTime;

      // Check for valid ISO8601
      if ((dateTime = parse8601Time(nodeP->value.s)) == -1)
      {
        LM_E(("parse8601Time failed"));
        ATTRIBUTE_ERROR("The 'observedAt' attribute must have a valid ISO8601 as value", NULL);
      }

      // Change to Number
      nodeP->type    = KjInt;
      nodeP->value.i = dateTime;

      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE17(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(observationSpaceP, "observation space", nodeP);
      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE15(nodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(operationSpaceP, "operation space", nodeP);
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
  }

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
      else if (geoJsonCheck(ciP, valueP, &details) == false)
      {
        LM_E(("geoJsonCheck error for %s: %s", caName, details));
        ATTRIBUTE_ERROR("geo-property attribute must have a valid GeoJson value", details);
      }

      caP->valueType       = orion::ValueTypeObject;
      caP->compoundValueP  = compoundCreate(ciP, valueP, NULL, 0);
    }
    else if (isTemporalProperty == true)
    {
      int64_t dateTime;

      if (valueP->type != KjString)
        ATTRIBUTE_ERROR("temporal-property attribute must have a value of type JSON String", kjValueType(valueP->type));
      else if ((dateTime = parse8601Time(valueP->value.s)) == -1)
        ATTRIBUTE_ERROR("temporal-property attribute must have a valid ISO8601 as value", NULL);

      caP->valueType   = orion::ValueTypeNumber;
      caP->numberValue = dateTime;
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
      case KjArray:      caP->valueType = orion::ValueTypeObject;  caP->compoundValueP = compoundCreate(ciP, valueP, NULL); break;
      case KjNull:       caP->valueType = orion::ValueTypeNull;    break;
      case KjNone:
        LM_E(("Invalid type from kjson"));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal error", "Invalid type from kjson", OrionldDetailsString);
        return false;
      }

      if (valueP->type == KjObject)
      {
        if (specialCompoundCheck(ciP, valueP) == false)
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
  // orionldContextTreat needs ceP to push the '@context' attribute to the ContextElement.
  //
  if (contextNodeP != NULL)
  {
    ContextAttribute* caP;

    if (orionldContextTreat(ciP, contextNodeP, idNodeP->value.s, &caP) == false)
    {
      // Error payload set by orionldContextTreat
      mongoRequest.release();
      return false;
    }

    ceP->contextAttributeVector.push_back(caP);
  }

  // Treat the entire payload
  for (KjNode* kNodeP = ciP->requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    LM_T(LmtUriExpansion, ("treating entity node '%s'", kNodeP->name));

    // Entity ID
    if (kNodeP == idNodeP)
    {
      if ((urlCheck(idNodeP->value.s, &details) == false) && (urnCheck(idNodeP->value.s, &details) == false))
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

      // FIXME: Call orionldUriExpand() - this here is a "copy" of what orionldUriExpand does
      extern int uriExpansion(OrionldContext* contextP, const char* name, char** expandedNameP, char** expandedTypeP, char** detailsPP);
      int    expansions = uriExpansion(ciP->contextP, typeNodeP->value.s, &expandedName, &expandedType, &details);
      LM_T(LmtUriExpansion, ("URI Expansion for type '%s': '%s'", typeNodeP->value.s, expandedName));
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
        LM_T(LmtUriExpansion, ("EXPANSION: use default URL for entity type '%s'", typeNodeP->value.s));
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
        LM_E(("attributeTreat failed"));
        ciP->httpStatusCode = SccBadRequest;  // FIXME: Should be set inside 'attributeTreat' - could be 500, not 400 ...
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
        char longName[256];

        if (orionldUriExpand(ciP->contextP, kNodeP->name, longName, sizeof(longName), &details) == false)
        {
          LM_E(("orionldUriExpand failed"));
          delete caP;
          mongoRequest.release();
          orionldErrorResponseCreate(ciP, OrionldBadRequestData, details, kNodeP->name, OrionldDetailsAttribute);
          return false;
        }

        caP->name = longName;
      }


      //
      // NO URI Expansion for Attribute TYPE
      //
      caP->type = attrTypeNodeP->value.s;

      ceP->contextAttributeVector.push_back(caP);
    }
  }

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
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Internal Error", "Error from mongo backend", OrionldDetailsString);
    return false;
  }

  ciP->httpStatusCode = SccCreated;
  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/entities/", idNodeP->value.s);
  httpHeaderLinkAdd(ciP, ciP->contextP, NULL);

  return true;
}



#if 0
      {
        LM_T(LmtUriExpansion, ("------------- URI-Expansion for attribute named '%s' starts here ------------------------------", kNodeP->name));
        char*  details;
        char*  expandedName  = NULL;
        char*  expandedType  = NULL;
        int    expansions    = uriExpansion(ciP->contextP, kNodeP->name, &expandedName, &expandedType, &details);

        LM_T(LmtUriExpansion, ("EXPANSION: uriExpansion returned %d for '%s'", expansions, kNodeP->name));
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
          LM_T(LmtUriExpansion, ("EXPANSION: use default URL for attribute '%s'", kNodeP->name));
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
#endif
  
