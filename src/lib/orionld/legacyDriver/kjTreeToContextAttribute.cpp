/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "ngsi/ContextAttribute.h"                               // ContextAttribute

#include "orionld/types/OrionldContext.h"                        // OrionldContext
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dateTime.h"                             // dateTimeFromString
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/payloadCheck/pcheckGeoPropertyValue.h"         // pcheckGeoPropertyValue
#include "orionld/payloadCheck/pcheckLanguagePropertyValue.h"    // pcheckLanguagePropertyValue
#include "orionld/legacyDriver/kjTreeToCompoundValue.h"          // kjTreeToCompoundValue
#include "orionld/legacyDriver/metadataAdd.h"                    // metadataAdd
#include "orionld/legacyDriver/kjTreeToMetadata.h"               // kjTreeToMetadata
#include "orionld/legacyDriver/kjTreeToContextAttribute.h"       // Own interface



// -----------------------------------------------------------------------------
//
// ATTRIBUTE_ERROR -
//
#define ATTRIBUTE_ERROR(title, details, attrName)                            \
do                                                                           \
{                                                                            \
  orionldError(OrionldBadRequestData, title, details, 400);                  \
  pdField(attrName);                                                         \
  return false;                                                              \
} while (0)



// -----------------------------------------------------------------------------
//
// specialCompoundCheck -
//
static bool specialCompoundCheck(const char* attrName, KjNode* compoundValueP)
{
  KjNode*  typeNodeP  = NULL;
  KjNode*  valueNodeP = NULL;
  KjNode*  otherNodeP = NULL;

  LM_W(("Attr name: '%s'", attrName));

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
      orionldError(OrionldBadRequestData, "unwanted extra items in @value/@type compound", otherNodeP->name, 400);
      return false;
    }

    // All info needed to check validity of @value
    if (SCOMPARE9(typeNodeP->value.s, 'D', 'a', 't', 'e', 'T', 'i', 'm', 'e', 0))
    {
      STRING_CHECK(valueNodeP, "@value of DateTime @type");

      char errorString[256];
      if (dateTimeFromString(valueNodeP->value.s, errorString, sizeof(errorString)) < 0)
      {
        orionldError(OrionldBadRequestData, "DateTime value of @value/@type compound must be a valid ISO8601", errorString, 400);
        LM_W(("Attr name: '%s'", attrName));
        pdAttribute(attrName);
        return false;
      }
    }
  }
  else if (valueNodeP != NULL)
  {
    if (otherNodeP != NULL)
    {
      orionldError(OrionldBadRequestData, "unwanted extra items in @value/@type compound", otherNodeP->name, 400);
      return false;
    }
  }
  else if (typeNodeP != NULL)
  {
    if (valueNodeP == NULL)
    {
      orionldError(OrionldBadRequestData, "missing @value in @value/@type compound", "@value is mandatory", 400);
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// attributeValueSet - set the value of a ContextAttribute instance
//
static bool attributeValueSet(ContextAttribute* caP, KjNode* valueP)
{
  switch (valueP->type)
  {
  case KjBoolean:    caP->valueType = orion::ValueTypeBoolean; caP->boolValue      = valueP->value.b; break;
  case KjInt:        caP->valueType = orion::ValueTypeNumber;  caP->numberValue    = valueP->value.i; break;
  case KjFloat:      caP->valueType = orion::ValueTypeNumber;  caP->numberValue    = valueP->value.f; break;
  case KjString:     caP->valueType = orion::ValueTypeString;  caP->stringValue    = valueP->value.s; break;
  case KjObject:     caP->valueType = orion::ValueTypeObject;  caP->compoundValueP = kjTreeToCompoundValue(valueP, NULL, 0); break;
  case KjArray:      caP->valueType = orion::ValueTypeObject;  caP->compoundValueP = kjTreeToCompoundValue(valueP, NULL, 0); break;
  case KjNull:       caP->valueType = orion::ValueTypeNull;    break;
  case KjNone:
    orionldError(OrionldBadRequestData, "Internal error", "Invalid JSON type", 400);
    return false;
  }

  if (valueP->type == KjObject)
  {
    LM_W(("Attr name: '%s'", caP->name.c_str()));
    if (specialCompoundCheck(caP->name.c_str(), valueP) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// atValueCheck -
//
static bool atValueCheck(KjNode* atTypeNodeP, KjNode* atValueNodeP, char** titleP, char** detailsP)
{
  // FIXME: Implement!!!
  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToContextAttribute -
//
// NOTE
//   Make sure that this function is not called for attributes called "createdAt" or "modifiedAt"
//
// FIXME: typeNodePP is currently used in orionldPatchEntity, orionldPostEntities, and orionldPostEntity as a means
//        to decide whether or not add the attribute to the attribute vector - "bool* ignoreP" would be better
//
//        Current fix is that the three callers of this function make sure that the function is NOT CALLED if the
//        name of the attribute is either "createdAt" or "modifiedAt" - that's why I could out-deff the initial part
//        that checks for those two names - faster solution.
//
// FIXME: This function is TOO LONG - try to split up
//
bool kjTreeToContextAttribute(OrionldContext* contextP, KjNode* kNodeP, ContextAttribute* caP, KjNode** typeNodePP, char** detailP)
{
  char* attributeName = kNodeP->name;

  caP->name    = attributeName;
  *detailP     = (char*) "unknown error";

  LM_W(("attributeName: '%s'", caP->name.c_str()));

  if (contextP == NULL)
    contextP = orionldCoreContextP;

  if (kNodeP->type != KjObject)
  {
    *detailP = (char*) "Attribute must be a JSON object";
    orionldError(OrionldBadRequestData, "Attribute must be a JSON object", attributeName, 400);
    return false;
  }

  //
  // For performance issues, all predefined names should have their char-sum precalculated
  // Advantage: just a simple integer comparison before we do the complete string-comparisom
  //
  KjNode*  typeP                  = NULL;  // For ALL:               Mandatory
  KjNode*  valueP                 = NULL;  // For 'Property':        Mandatory
  KjNode*  objectP                = NULL;  // For 'Relationship:     Mandatory
  KjNode*  languageMapP           = NULL;  // For 'LanguageProperty: Mandatory
  KjNode*  unitCodeP              = NULL;  // For 'Property':        Optional
  KjNode*  observedAtP            = NULL;  // For ALL:               Optional
  KjNode*  locationP              = NULL;
  KjNode*  observationSpaceP      = NULL;
  KjNode*  operationSpaceP        = NULL;
  KjNode*  creDateP               = NULL;
  KjNode*  modDateP               = NULL;
  bool     isProperty             = false;
  bool     isGeoProperty          = false;
  bool     isLanguageProperty     = false;
  bool     isTemporalProperty     = false;
  bool     isRelationship         = false;
  KjNode*  nodeP                  = kNodeP->value.firstChildP;

  while (nodeP != NULL)
  {
    if (SCOMPARE5(nodeP->name, 't', 'y', 'p', 'e', 0))
    {
      //
      // Duplicate check
      //
      if (typeP != NULL)
      {
        *detailP = (char*) "Duplicated field";
        orionldError(OrionldBadRequestData, "Duplicated field", "Attribute Type", 400);
        return false;
      }
      typeP = nodeP;

      //
      // Must be a JSON String
      //
      if (typeP->type != KjString)
      {
        *detailP = (char*) "Attribute type must be a JSON String";
        orionldError(OrionldBadRequestData, "Not a JSON String", "attribute type", 400);
        orionldState.httpStatusCode = 400;
        return false;
      }

      if (strcmp(nodeP->value.s, "Property") == 0)
        isProperty = true;
      else if (strcmp(nodeP->value.s, "Relationship") == 0)
        isRelationship = true;
      else if (strcmp(nodeP->value.s, "LanguageProperty") == 0)
      {
        if (experimental == true)
        {
          isProperty         = true;
          isLanguageProperty = true;
        }
        else
        {
          orionldError(OrionldBadRequestData, "Invalid Attribute Type", "LanguageProperty", 501);
          return false;
        }
      }
      else if (strcmp(nodeP->value.s, "GeoProperty") == 0)
      {
        isProperty    = true;
        isGeoProperty = true;

        if (orionldState.geoAttrs >= orionldState.geoAttrMax)
        {
          orionldState.geoAttrMax += 100;

          KjNode** tmp = (KjNode**) kaAlloc(&orionldState.kalloc, sizeof(KjNode*) * orionldState.geoAttrMax);

          if (tmp == NULL)
          {
            orionldError(OrionldBadRequestData, "Internal Error", "Unable to allocate memory", 500);
            return false;
          }

          memcpy(tmp, orionldState.geoAttrV, sizeof(KjNode*) * orionldState.geoAttrs);
          orionldState.geoAttrV = tmp;
        }

        orionldState.geoAttrV[orionldState.geoAttrs++] = kNodeP;
      }
#if 0
      else if (strcmp(nodeP->value.s, "TemporalProperty") == 0)
      {
        // Not even in t he NGSI-LD API spec yet ...
        isProperty         = true;
        isTemporalProperty = true;
      }
#endif
      else
      {
        *detailP = (char*) "Invalid type for attribute";
        LM_E(("Invalid type for attribute '%s': '%s'", nodeP->name, nodeP->value.s));
        orionldError(OrionldBadRequestData, "Invalid type for attribute", nodeP->value.s, 400);
        return false;
      }

      if (typeNodePP != NULL)
        *typeNodePP = typeP;
      caP->type   = typeP->value.s;
    }
    else if (SCOMPARE6(nodeP->name, 'v', 'a', 'l', 'u', 'e', 0))
    {
      DUPLICATE_CHECK(valueP, "attribute value", nodeP);
      // FIXME: "value" for Relationship Attribute should be added as metadata
    }
    else if (SCOMPARE9(nodeP->name, 'u', 'n', 'i', 't', 'C', 'o', 'd', 'e', 0))
    {
      DUPLICATE_CHECK(unitCodeP, "unit code", nodeP);
      STRING_CHECK(unitCodeP, "unitCode");
      if (metadataAdd(caP, nodeP, attributeName) == false)
      {
        // metadataAdd calls orionldError
        *detailP = (char*) "metadataAdd failed";
        LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
        return false;
      }
    }
    else if (SCOMPARE7(nodeP->name, 'o', 'b', 'j', 'e', 'c', 't', 0))
    {
      DUPLICATE_CHECK(objectP, "object", nodeP);
    }
    else if (strcmp(nodeP->name, "languageMap") == 0)
    {
      DUPLICATE_CHECK(languageMapP, "languageMap", nodeP);
      // OBJECT_CHECK(languageMapP, "languageMap"); - better error text inside pcheckLanguagePropertyValue
      valueP = languageMapP;
    }
    else if (SCOMPARE11(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
    {
      DUPLICATE_CHECK(observedAtP, "observed at", nodeP);
      STRING_CHECK(nodeP, "observedAt");

      //
      // This is a very special attribute.
      // It's value must be a JSON String and the string must be a valid ISO8601 dateTime
      // The string is changed for a Number before stored in the database
      //
      double dateTime;

      // Check for valid ISO8601
      char errorString[256];
      if ((dateTime = dateTimeFromString(nodeP->value.s, errorString, sizeof(errorString))) < 0)
        ATTRIBUTE_ERROR("Invalid ISO8601", errorString, "observedAt");

      // Change to Number
      nodeP->type    = KjFloat;
      nodeP->value.f = dateTime;

      if (metadataAdd(caP, nodeP, attributeName) == false)
      {
        orionldError(OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, 400);
        *detailP = (char*) "Error adding metadata 'observed at' to attribute";
        return false;
      }
    }
    else if (SCOMPARE9(nodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(locationP, "location", nodeP);
      if (metadataAdd(caP, nodeP, attributeName) == false)
      {
        *detailP = (char*) "Error adding metadata 'location' to attribute";
        orionldError(OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, 400);
        return false;
      }
    }
    else if (SCOMPARE17(nodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(observationSpaceP, "observation space", nodeP);
      if (metadataAdd(caP, nodeP, attributeName) == false)
      {
        *detailP = (char*) "Error adding metadata 'observation space' to attribute";
        orionldError(OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, 400);
        return false;
      }
    }
    else if (SCOMPARE15(nodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(operationSpaceP, "operation space", nodeP);
      if (metadataAdd(caP, nodeP, attributeName) == false)
      {
        *detailP = (char*) "Error adding metadata 'operation space' to attribute";
        orionldError(OrionldBadRequestData, "Error adding metadata to attribute", nodeP->name, 400);
        return false;
      }
    }
    else if (SCOMPARE8(nodeP->name, 'c', 'r', 'e', 'D', 'a', 't', 'e', 0))
    {
      DUPLICATE_CHECK(creDateP, "creDate", nodeP);
      caP->creDate = nodeP->value.f;
    }
    else if (SCOMPARE8(nodeP->name, 'm', 'o', 'd', 'D', 'a', 't', 'e', 0))
    {
      DUPLICATE_CHECK(modDateP, "modDate", nodeP);
      caP->modDate = nodeP->value.f;
    }
    else if (SCOMPARE8(nodeP->name, 'm', 'd', 'N', 'a', 'm', 'e', 's', 0))
    {
    }
    else if (SCOMPARE3(nodeP->name, 'm', 'd', 0))
    {
      for (KjNode* mdP = nodeP->value.firstChildP; mdP != NULL; mdP = mdP->next)
      {
        if (kjTreeToMetadata(caP, mdP, attributeName, detailP) == false)
          return false;
      }
    }
    else  // Other
    {
      if (kjTreeToMetadata(caP, nodeP, attributeName, detailP) == false)
        return false;
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
    orionldError(OrionldBadRequestData, "Attribute found, but the type field is missing", attributeName, 400);
    *detailP = (char*) "Attr Type is mandatory";
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
        orionldError(OrionldBadRequestData, "Attribute with type GeoProperty found, but the associated value field is missing", attributeName, 400);
        *detailP = (char*) "value missing for GeoProperty";
      }
      else if (isLanguageProperty == true)
      {
        LM_E(("value missing for LanguageProperty '%s'", kNodeP->name));
        orionldError(OrionldBadRequestData, "Attribute with type LanguageProperty found, but the associated value field is missing", attributeName, 400);
        *detailP = (char*) "value missing for LanguageProperty";
      }
      else if (isTemporalProperty == true)
      {
        LM_E(("value missing for TemporalProperty '%s'", kNodeP->name));
        orionldError(OrionldBadRequestData, "Attribute with type TemporalProperty found, but the associated value field is missing", attributeName, 400);
        *detailP = (char*) "value missing for TemporalProperty";
      }
      else
      {
        LM_E(("value missing for Property '%s'", kNodeP->name));
        orionldError(OrionldBadRequestData, "Attribute with type Property found, but the associated value field is missing", attributeName, 400);
        *detailP = (char*) "value missing for Property";
      }

      return false;
    }

    if (valueP->type == KjNull)
    {
      LM_E(("NULL value for Property '%s'", kNodeP->name));
      orionldError(OrionldBadRequestData, "Attributes with type Property cannot be given the value NULL", attributeName, 400);
      *detailP = (char*) "NULL value for Property";
      return NULL;
    }

    //
    // Get the value, and check for correct value for Geo/Temporal Attributes
    //
    if (isGeoProperty == true)
    {
      if (pcheckGeoPropertyValue(valueP, &orionldState.geoType, &orionldState.geoCoordsP, attributeName) == false)
      {
        // pcheckGeoPropertyValue fills in error response
        *detailP = (char*) "pcheckGeoProperty failed";
        orionldState.httpStatusCode = 400;
        return false;
      }
      caP->valueType       = orion::ValueTypeObject;
      caP->compoundValueP  = kjTreeToCompoundValue(valueP, NULL, 0);
    }
    else if (isLanguageProperty == true)
    {
      if (pcheckLanguagePropertyValue(valueP, attributeName) == false)
      {
        LM_E(("pcheckLanguageProperty error for %s", attributeName));
        // pcheckLanguageProperty fills in error response
        *detailP = (char*) "pcheckLanguageProperty failed";
        orionldState.httpStatusCode = 400;
        return false;
      }

      caP->valueType       = orion::ValueTypeObject;
      caP->compoundValueP  = kjTreeToCompoundValue(valueP, NULL, 0);
    }
    else if (isTemporalProperty == true)
    {
      //
      // Can be either simply a string (then DateTime is assumed, or an object with @type and @value
      // If object, then @type can be any of these three:
      //   o DateTime
      //   o Date
      //   o Time
      //
      // The @value must be a string and must be valid according to @type
      //
      if (valueP->type == KjString)
      {
        double dateTime;
        char   errorString[256];

        if ((dateTime = dateTimeFromString(valueP->value.s, errorString, sizeof(errorString))) < 0)
        {
          *detailP = (char*) errorString;
          ATTRIBUTE_ERROR("temporal property must have a valid ISO8601 as value", errorString, kNodeP->name);
        }

        caP->numberValue = dateTime;
        caP->valueType   = orion::ValueTypeNumber;
      }
      else if (valueP->type == KjObject)
      {
        KjNode* atTypeNodeP   = NULL;
        KjNode* atValueNodeP  = NULL;

        for (KjNode* nodeP = valueP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
        {
          if (SCOMPARE6(nodeP->name, '@', 't', 'y', 'p', 'e', 0))
          {
            DUPLICATE_CHECK(atTypeNodeP, "@type", nodeP);
            STRING_CHECK(nodeP, "@type");
          }
          else if (SCOMPARE7(nodeP->name, '@', 'v', 'a', 'l', 'u', 'e', 0))
          {
            DUPLICATE_CHECK(atValueNodeP, "@value", nodeP);
            STRING_CHECK(nodeP, "@value");
          }
          else
          {
            LM_E(("Invalid member of value as object of Temporal Property: '%s'", nodeP->name));
            orionldError(OrionldBadRequestData, "Invalid member of value as object of Temporal Property", nodeP->name, 400);
            *detailP = (char*) "Invalid member of value as object of Temporal Property";
            return false;
          }
        }

        if (atValueNodeP == NULL)
        {
          LM_E(("@value node missing in value-object of Temporal Property '%s'", valueP->name));
          orionldError(OrionldBadRequestData, "@value node missing in value-object of Temporal Property", valueP->name, 400);
          *detailP = (char*) "@value node missing in value-object of Temporal Property";
          return false;
        }

        char* title;
        char* details;
        if ((atTypeNodeP != NULL) && (atValueCheck(atTypeNodeP, atValueNodeP, &title, &details) == false))
        {
          LM_E(("Invalid temporal value of Temporal Property '%s'", valueP->name));
          orionldError(OrionldBadRequestData, title, details, 400);
          *detailP = (char*) "Invalid temporal value of Temporal Property";
          return false;
        }

        orion::CompoundValueNode* cNodeP      = new orion::CompoundValueNode();
        orion::CompoundValueNode* cValueNodeP = new orion::CompoundValueNode();

        cNodeP->valueType        = orion::ValueTypeObject;

        cValueNodeP->name        = "@value";
        cValueNodeP->valueType   = orion::ValueTypeNumber;

        char   errorString[256];
        double dateTime = dateTimeFromString(atValueNodeP->value.s, errorString, sizeof(errorString));
        if (dateTime < 0)
        {
          orionldError(OrionldBadRequestData, "Invalid ISO8601", errorString, 400);
          *detailP =  errorString;
          return false;
        }

        cValueNodeP->numberValue = dateTime;  // FIXME: Assuming "DateTime" - "Date"/"Time" ...
        cNodeP->childV.push_back(cValueNodeP);

        if (atTypeNodeP != NULL)
        {
          orion::CompoundValueNode* cTypeNodeP  = new orion::CompoundValueNode();

          cTypeNodeP->name        = "@type";
          cTypeNodeP->valueType   = orion::ValueTypeString;
          cTypeNodeP->stringValue = atTypeNodeP->value.s;

          cNodeP->childV.push_back(cTypeNodeP);
        }

        caP->valueType      = orion::ValueTypeObject;
        caP->compoundValueP = cNodeP;
      }
      else
      {
        *detailP = (char*) "temporal-property attribute must have a value of type JSON String or a JSON object with @value and @type";
        ATTRIBUTE_ERROR("temporal-property attribute must have a value of type JSON String or a JSON object with @value and @type", kjValueType(valueP->type), caP->name.c_str());
      }
    }
    else
    {
      LM_W(("Calling attributeValueSet for attribute '%s'", caP->name.c_str()));
      if (attributeValueSet(caP, valueP) == false)
      {
        // attributeValueSet calls orionldError
        *detailP = (char*) "attributeValueSet failed";
        return false;
      }
    }
  }
  else if (isRelationship == true)
  {
    if (objectP == NULL)
      ATTRIBUTE_ERROR("Invalid attribute", "relationship without 'object'", attributeName);

    if (objectP->type == KjString)
    {
      if (pCheckUri(objectP->value.s, "object", true) == false)
        ATTRIBUTE_ERROR("Relationship attribute with 'object' field having invalid URI", objectP->value.s, attributeName);

      caP->valueType   = orion::ValueTypeString;
      caP->stringValue = objectP->value.s;
    }
    else if (objectP->type == KjArray)
    {
      for (KjNode* nodeP = objectP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
      {
        if (nodeP->type != KjString)
          ATTRIBUTE_ERROR("Relationship attribute with 'object' array item not being a JSON string", kjValueType(nodeP->type), attributeName);

        char* uri = nodeP->value.s;
        if (pCheckUri(uri, "object array item", true) == false)
          ATTRIBUTE_ERROR("Relationship attribute with 'object array' field having invalid URI", uri, attributeName);
      }
      caP->valueType      = orion::ValueTypeVector;
      caP->compoundValueP = kjTreeToCompoundValue(objectP, NULL, 0);
    }
    else
      ATTRIBUTE_ERROR("Invalid attribute", "relationship with 'object' field of invalid type (must be a String or an Array or Strings)", attributeName);
  }

  return true;
}
