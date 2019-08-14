/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "ngsi/ContextAttribute.h"                               // ContextAttribute
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "orionld/common/geoJsonCheck.h"                         // geoJsonCheck
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/urlCheck.h"                             // urlCheck
#include "orionld/common/urnCheck.h"                             // urnCheck
#include "orionld/common/orionldAttributeTreat.h"                // Own interface



// -----------------------------------------------------------------------------
//
// ATTRIBUTE_ERROR -
//
#define ATTRIBUTE_ERROR(errorString, details)                                                        \
do                                                                                                   \
{                                                                                                    \
  LM_E((errorString));                                                                               \
  orionldErrorResponseCreate(OrionldBadRequestData, errorString, details, OrionldDetailsAttribute);  \
  ciP->httpStatusCode = SccBadRequest;                                                               \
  return false;                                                                                      \
} while (0)



// -----------------------------------------------------------------------------
//
// Forward declaration - compoundCreate is used by compoundValueNodeValueSet
//                       and compoundValueNodeValueSet uses compoundCreate
//
static orion::CompoundValueNode* compoundCreate(ConnectionInfo* ciP, KjNode* kNodeP, KjNode* parentP, int level = 0);



// -----------------------------------------------------------------------------
//
// compoundValueNodeValueSet - set the value of a CompoundeValueNode instance
//
static bool compoundValueNodeValueSet(ConnectionInfo* ciP, orion::CompoundValueNode* cNodeP, KjNode* kNodeP, int* levelP)
{
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
    *levelP += 1;
    cNodeP->valueType = orion::ValueTypeObject;

    for (KjNode* kChildP = kNodeP->value.firstChildP; kChildP != NULL; kChildP = kChildP->next)
    {
      orion::CompoundValueNode* cChildP = compoundCreate(ciP, kChildP, kNodeP, *levelP);
      cNodeP->childV.push_back(cChildP);
    }
  }
  else if (kNodeP->type == KjArray)
  {
    *levelP += 1;
    cNodeP->valueType = orion::ValueTypeVector;

    for (KjNode* kChildP = kNodeP->value.firstChildP; kChildP != NULL; kChildP = kChildP->next)
    {
      orion::CompoundValueNode* cChildP = compoundCreate(ciP, kChildP, kNodeP, *levelP);

      cNodeP->childV.push_back(cChildP);
    }
  }
  else
  {
    LM_E(("Invalid json type (KjNone!) for value field of compound '%s'", cNodeP->name.c_str()));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal error", "Invalid type from kjson", OrionldDetailsString);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// compoundCreate -
//
static orion::CompoundValueNode* compoundCreate(ConnectionInfo* ciP, KjNode* kNodeP, KjNode* parentP, int level)
{
  if (kNodeP->type != KjArray)
    LM_T(LmtCompoundCreation, ("In compoundCreate: creating '%s' called '%s' on level %d", kjValueType(kNodeP->type), kNodeP->name, level));
  else
    LM_T(LmtCompoundCreation, ("In compoundCreate: creating '%s' on level %d", kjValueType(kNodeP->type), level));

  orion::CompoundValueNode* cNodeP = new orion::CompoundValueNode();

  if ((parentP != NULL) && (parentP->type == KjObject))
    cNodeP->name = kNodeP->name;

  if (compoundValueNodeValueSet(ciP, cNodeP, kNodeP, &level) == false)
  {
    // compoundValueNodeValueSet calls orionldErrorResponseCreate
    return NULL;
  }

  return cNodeP;
}



// -----------------------------------------------------------------------------
//
// specialCompoundCheck -
//
static bool specialCompoundCheck(ConnectionInfo* ciP, KjNode* compoundValueP)
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
      orionldErrorResponseCreate(OrionldBadRequestData, "unwanted extra items in @value/@type compound", otherNodeP->name, OrionldDetailsString);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }

    // All info needed to check validity of @value
    if (SCOMPARE9(typeNodeP->value.s, 'D', 'a', 't', 'e', 'T', 'i', 'm', 'e', 0))
    {
      STRING_CHECK(valueNodeP, "@value of DateTime @type");

      if (parse8601Time(valueNodeP->value.s) == -1)
        ATTRIBUTE_ERROR("DateTime value of @value/@type compound must be a valid ISO8601", compoundValueP->name);
    }
  }
  else if (valueNodeP != NULL)
  {
    if (otherNodeP != NULL)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "unwanted extra items in @value/@type compound", otherNodeP->name, OrionldDetailsString);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }
  else if (typeNodeP != NULL)
  {
    if (valueNodeP == NULL)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "missing @value in @value/@type compound", "@value is mandatory", OrionldDetailsString);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// attributeValueSet - set the value of a ContextAttribute instance
//
static bool attributeValueSet(ConnectionInfo* ciP, ContextAttribute* caP, KjNode* valueP)
{
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
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal error", "Invalid type from kjson", OrionldDetailsString);
    ciP->httpStatusCode = SccReceiverInternalError;
    return false;
  }

  if (valueP->type == KjObject)
  {
    if (specialCompoundCheck(ciP, valueP) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// metadataValueSet - set the value of a Metadata instance
//
static bool metadataValueSet(ConnectionInfo* ciP, Metadata* mdP, KjNode* valueNodeP)
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
    LM_E(("Invalid json type (KjNone!) for value field of metadata '%s'", valueNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal error", "Invalid type from kjson", OrionldDetailsString);
    ciP->httpStatusCode = SccReceiverInternalError;
    return false;
  }

  return true;
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
          isProperty = true;
        else if (SCOMPARE13(kNodeP->value.s, 'R', 'e', 'l', 'a', 't', 'i', 'o', 'n', 's', 'h', 'i', 'p', 0))
          isRelationship = true;
        else
        {
          LM_E(("Invalid type for metadata '%s': '%s'", kNodeP->name, kNodeP->value.s));
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid type for attribute", kNodeP->value.s, OrionldDetailsString);
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
      LM_E(("No type for metadata '%s'", nodeP->name));
      orionldErrorResponseCreate(OrionldBadRequestData, "The type field is missing for a metadata", nodeP->name, OrionldDetailsString);
      return false;
    }

    if ((isProperty == true) && (valueNodeP == NULL))
    {
      LM_E(("No value for Property metadata '%s'", nodeP->name));
      orionldErrorResponseCreate(OrionldBadRequestData, "The value field is missing for a Property metadata", nodeP->name, OrionldDetailsString);
      return false;
    }
    else if ((isRelationship == true) && (objectNodeP == NULL))
    {
      LM_E(("No 'object' for Relationship metadata '%s'", nodeP->name));
      orionldErrorResponseCreate(OrionldBadRequestData, "The value field is missing for a Relationship metadata", nodeP->name, OrionldDetailsString);
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
    orionldErrorResponseCreate(OrionldInternalError, "cannot create property/relationship for attribute", "out of memory", OrionldDetailsString);
    return false;
  }

  mdP->name = nodeP->name;

  if (typeNodeP != NULL)  // Only if the metadata is a JSON Object
    mdP->type = typeNodeP->value.s;

  if (isProperty == true)
  {
    if (metadataValueSet(ciP, mdP, valueNodeP) == false)
    {
      // metadataValueSet calls metadataValueSet
      delete mdP;
      return false;
    }
  }
  else  // Relationship
  {
    // A "Relationship" has no value, instead it has 'object', that must be of string type
    if (objectNodeP->type != KjString)
    {
      LM_E(("invalid json type for relationship-object '%s' of attribute '%s'", nodeP->name, caName));
      orionldErrorResponseCreate(OrionldInternalError, "invalid json type for relationship-object", nodeP->name, OrionldDetailsString);
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
// orionldAttributeTreat -
//
bool orionldAttributeTreat(ConnectionInfo* ciP, KjNode* kNodeP, ContextAttribute* caP, KjNode** typeNodePP)
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

        if (orionldState.locationAttributeP != NULL)
        {
          orionldErrorResponseCreate(OrionldBadRequestData, "Multiple attributes cannot be defined using the GeoProperty type", nodeP->name, OrionldDetailsString);
          return false;
        }

        orionldState.locationAttributeP = kNodeP;
      }
      else if (SCOMPARE17(nodeP->value.s, 'T', 'e', 'm', 'p', 'o', 'r', 'a', 'l', 'P', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
      {
        isProperty         = true;
        isTemporalProperty = true;
      }
      else
      {
        LM_E(("Invalid type for attribute '%s': '%s'", nodeP->name, nodeP->value.s));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid type for attribute", nodeP->value.s, OrionldDetailsString);
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
        orionldErrorResponseCreate(OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
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
        ATTRIBUTE_ERROR("The 'observedAt' attribute must have a valid ISO8601 as value", nodeP->name);
      }

      // Change to Number
      nodeP->type    = KjInt;
      nodeP->value.i = dateTime;

      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE17(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(observationSpaceP, "observation space", nodeP);
      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else if (SCOMPARE15(nodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(operationSpaceP, "operation space", nodeP);
      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, OrionldDetailsString);
        return false;
      }
    }
    else  // Other
    {
      if (caP->metadataVector.lookupByName(nodeP->name) != NULL)
      {
        LM_E(("Duplicated attribute property '%s' for attribute '%s'", nodeP->name, caP->name.c_str()));
        orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated attribute property", nodeP->name, OrionldDetailsString);
        return false;
      }

      if (metadataAdd(ciP, caP, nodeP, caName) == false)
      {
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Error adding metadata to an attribute", nodeP->name, OrionldDetailsString);
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
    LM_E(("type missing for attribute '%s'", kNodeP->name));
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute found, but the type field is missing", kNodeP->name, OrionldDetailsAttribute);
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
        LM_E(("value missing for GeoProperty '%s'", kNodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Attribute with type GeoProperty found, but the associated value field is missing", kNodeP->name, OrionldDetailsAttribute);
      }
      else if (isTemporalProperty == true)
      {
        LM_E(("value missing for TemporalProperty '%s'", kNodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Attribute with type TemporalProperty found, but the associated value field is missing", kNodeP->name, OrionldDetailsAttribute);
      }
      else
      {
        LM_E(("value missing for Property '%s'", kNodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Attribute with type Property found, but the associated value field is missing", kNodeP->name, OrionldDetailsAttribute);
      }

      return false;
    }

    if (valueP->type == KjNull)
    {
      LM_E(("NULL value for Property '%s'", kNodeP->name));
      orionldErrorResponseCreate(OrionldBadRequestData, "Attributes with type Property cannot be given the value NULL", kNodeP->name, OrionldDetailsAttribute);
      return NULL;
    }

    //
    // Get the value, and check for correct value for Geo/Temporal Attributes
    //
    if (isGeoProperty == true)
    {
      char* details = (char*) "no details";

      if (valueP->type != KjObject)
      {
        LM_E(("geo-property attribute value must be a JSON Object: %s", kNodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "The value of an attribute of type GeoProperty must be a JSON Object", kNodeP->name, OrionldDetailsString);
        return false;
      }

      if (geoJsonCheck(ciP, valueP, &details) == false)
      {
        LM_E(("geoJsonCheck error for %s: %s", caName, details));
        orionldErrorResponseCreate(OrionldBadRequestData, "The value of an attribute of type GeoProperty must be valid GeoJson", details, OrionldDetailsString);
        return false;
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
        ATTRIBUTE_ERROR("temporal-property attribute must have a valid ISO8601 as value", kNodeP->name);

      caP->valueType   = orion::ValueTypeNumber;
      caP->numberValue = dateTime;
    }
    else
    {
      if (attributeValueSet(ciP, caP, valueP) == false)
      {
        // attributeValueSet calls orionldErrorResponseCreate
        return false;
      }
    }
  }
  else if (isRelationship == true)
  {
    char* details;

    if (objectP == NULL)
      ATTRIBUTE_ERROR("relationship attribute without 'object' field", caName);

    if (objectP->type == KjString)
    {
      if ((urlCheck(objectP->value.s, &details) == false) && (urnCheck(objectP->value.s, &details) == false))
        ATTRIBUTE_ERROR("relationship attribute with 'object' field having invalid URI", objectP->value.s);

      caP->valueType   = orion::ValueTypeString;
      caP->stringValue = objectP->value.s;
    }
    else if (objectP->type == KjArray)
    {
      for (KjNode* nodeP = objectP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
      {
        if (nodeP->type != KjString)
          ATTRIBUTE_ERROR("relationship attribute with 'object' array item having invalid URI", objectP->value.s);

        char* uri = nodeP->value.s;
        if ((urlCheck(uri, &details) == false) && (urnCheck(uri, &details) == false))
          ATTRIBUTE_ERROR("relationship attribute with 'object array' field having invalid URI", objectP->value.s);
      }
      caP->valueType      = orion::ValueTypeVector;
      caP->compoundValueP = compoundCreate(ciP, objectP, NULL);
    }
    else
      ATTRIBUTE_ERROR("relationship attribute with 'object' field of invalid type (must be a String or an Array or Strings)", caName);
  }

  return true;
}
