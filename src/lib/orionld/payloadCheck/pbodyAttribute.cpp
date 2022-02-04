/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/types/OrionldResponseErrorType.h"              // OrionldBadRequestData
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType
#include "orionld/types/OrionldAttributeType.h"                  // orionldAttributeTypeName
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/pbodyAttribute.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// kjObjectTransform -
//
static inline void kjObjectTransform(KjNode* attrP, KjNode* firstChildP, KjNode* lastChildP)
{
  attrP->type              = KjObject;
  attrP->value.firstChildP = firstChildP;
  attrP->lastChild         = lastChildP;
}



// -----------------------------------------------------------------------------
//
// pcheckAttributeTypeValue -
//
bool pcheckAttributeTypeValue
(
  KjNode*                 typeP,
  OrionldAttributeType*   attributeTypeP,
  OrionldProblemDetails*  pdP
)
{
  if (typeP->type != KjString)
  {
    orionldError(pdP, OrionldBadRequestData, "Invalid attribute type field", "Must be a String", 400);
    return false;
  }

  if      (strcmp(typeP->value.s, "Property")          == 0) *attributeTypeP = Property;
  else if (strcmp(typeP->value.s, "Relationship")      == 0) *attributeTypeP = Relationship;
  else if (strcmp(typeP->value.s, "GeoProperty")       == 0) *attributeTypeP = GeoProperty;
  else if (strcmp(typeP->value.s, "LanguageProperty")  == 0) *attributeTypeP = LanguageProperty;
  else
  {
    orionldError(pdP, OrionldBadRequestData, "Invalid value for attribute type field", typeP->value.s, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pcheckAttributeType -
//
bool pcheckAttributeType
(
  KjNode*                 typeP,
  KjNode*                 valueP,
  KjNode*                 objectP,
  KjNode*                 languageMapP,
  OrionldAttributeType*   attributeTypeP,
  OrionldProblemDetails*  pdP
)
{
  if ((typeP != NULL) && (pcheckAttributeTypeValue(typeP, attributeTypeP, pdP) == false))
    return false;

  if (*attributeTypeP == Property)
  {
    if (objectP != NULL)
    {
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A Property cannot have an /object/ field",
                   400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A Property cannot have a /languageMap/ field",
                   400);
      return false;
    }
  }
  else if (*attributeTypeP == GeoProperty)
  {
    if (objectP != NULL)
    {
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A GeoProperty cannot have an /object/ field",
                   400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A GeoProperty cannot have a /languageMap/ field",
                   400);
      return false;
    }
  }
  else if (*attributeTypeP == Relationship)
  {
    if (valueP != NULL)
    {
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A Relationship cannot have a /value/ field",
                   400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A Relationship cannot have a /languageMap/ field",
                   400);
      return false;
    }
  }
  else if (*attributeTypeP == LanguageProperty)
  {
    if (valueP != NULL)
    {
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A LanguageMap cannot have a /value/ field",
                   400);
      return false;
    }
    else if (objectP != NULL)
    {
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A LanguageMap cannot have an /object/ field",
                   400);
      return false;
    }
  }

  return true;
}



//static bool pcheckLanguagePropertyValue(KjNode* attrP, OrionldProblemDetails* pdP) { return true; }
static bool pcheckGeoPropertyValue(KjNode* attrP, OrionldProblemDetails* pdP) { return true; }
//static bool pcheckRelationshipObject(KjNode* attrP, OrionldProblemDetails* pdP) { return true; }
//static bool pcheckPropertyValue(KjNode* attrP, OrionldProblemDetails* pdP) { return true; }


#if 0
// -----------------------------------------------------------------------------
//
// attributeTypeGuess -
//
OrionldAttributeType attributeTypeGuess
(
  KjNode*                 attrP,
  KjNode*                 valueP,
  KjNode*                 objectP,
  KjNode*                 languageMapP,
  OrionldProblemDetails*  pdP
)
{
  LM_TMP(("KZ: valueP at       %p", valueP));
  LM_TMP(("KZ: objectP at      %p", objectP));
  LM_TMP(("KZ: languageMapP at %p", languageMapP));

  if (objectP != NULL)
    return Relationship;
  LM_TMP(("KZ: NOY directly a Relationship"));

  if (languageMapP != NULL)
    return LanguageProperty;

  // GeoProperty?
  if (valueP != NULL)
  {
    if ((valueP->type == KjObject) && (kjLookup(valueP, "type") != NULL) && (kjLookup(valueP, "coordinates") != NULL))
      return GeoProperty;

    return Property;
  }

  //
  // No "value", nor "object", nor "languageMap" - must be a pure key-value
  //
  // Can no longer be a Relationship - no "object"
  // Can no longer be a LanguageProperty - no "languageMap"
  // Might be a GeoProperty, if "type" and "coordinates" are present
  // Might be a Property - it probably is
  //

  if ((kjLookup(attrP, "type") != NULL) && (kjLookup(attrP, "coordinates") != NULL))
    return GeoProperty;

  return Property;
}



// -----------------------------------------------------------------------------
//
// pbodyAttributeAsObject -
//
static bool pbodyAttributeAsObject(KjNode* attrP, OrionldProblemDetails* pdP)
{
  KjNode*               typeP         = kjLookup(attrP, "type");
  KjNode*               valueP        = kjLookup(attrP, "value");
  KjNode*               objectP       = kjLookup(attrP, "object");
  KjNode*               languageMapP  = kjLookup(attrP, "languageMap");
  OrionldAttributeType  attributeType = NoAttributeType;

  //
  // If "type" is missing ... we add it if we can
  // We'll know after the loop
  // for (KjNode* fieldP = attrP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next) ...
  //
  if (typeP != NULL)
  {
    LM_TMP(("KZ: Got a type member of the attribute RHS Object"));

    // Might be a GeoProperty value. If so, also "coordinates" needs to be present
    KjNode* coordinatesP = kjLookup(attrP, "coordinates");
    if (coordinatesP != NULL)
    {
      attributeType = GeoProperty;
      LM_TMP(("KZ: YES, it's a GeoProperty!"));

      //
      // The "type" we found was not an attribute type - it was the type of the GeoProperty (Point, Polygon, ...)
      // For the rest of the function to work correctly, we need to NULL out that info - no attribute type (Property, Relatiopnship, ...) was actually found!
      //
      typeP = NULL;
    }
    else if (pcheckAttributeType(typeP, valueP, objectP, languageMapP, &attributeType, pdP) == false)
      return false;

    if ((attributeType == Relationship) && (pcheckRelationshipObject(objectP, pdP) == false))
      return false;
    else if ((attributeType == GeoProperty) && (pcheckGeoPropertyValue(valueP, pdP) == false))
      return false;
    else if ((attributeType == LanguageProperty) && (pcheckLanguagePropertyValue(languageMapP, pdP) == false))
      return false;
  }
  else
  {
    LM_TMP(("KZ: Guessing attribute type for '%s'", attrP->name));
    attributeType = attributeTypeGuess(attrP, valueP, objectP, languageMapP, pdP);
    LM_TMP(("KZ: Guessed attribute type %d: %s", attributeType, orionldAttributeTypeName[attributeType]));
    if (attributeType == NoAttributeType)
      return false;

    if ((attributeType == Relationship) && (pcheckRelationshipObject(objectP, pdP) == false))
      return false;
    else if ((attributeType == LanguageProperty) && (pcheckLanguagePropertyValue(languageMapP, pdP) == false))
      return false;
    else if (valueP != NULL)
    {
      if ((attributeType == GeoProperty) && (pcheckGeoPropertyValue(valueP, pdP) == false))
        return false;
      else if ((attributeType == Property) && (pcheckPropertyValue(valueP, pdP) == false))
        return false;
    }
  }

  LM_TMP(("KZ: still here ..."));
  //
  //
  //
  if ((attributeType == Property) || (attributeType == GeoProperty))
  {
    if (valueP == NULL)  // The entire tree is the value
    {
      LM_TMP(("KZ: no 'value' member - the entire tree is the value"));
      valueP = kjObject(orionldState.kjsonP, "value");
      valueP->value.firstChildP = attrP->value.firstChildP;
      valueP->lastChild         = attrP->lastChild;

      kjObjectTransform(attrP, valueP, valueP);
    }
    else  // We have a value ...
    {
      LM_TMP(("KZ: 'value' member present. Add type?"));
    }
  }

  if (typeP == NULL)  // Add type if not there
  {
    typeP = kjString(orionldState.kjsonP, "type", orionldAttributeTypeName[attributeType]);
    kjChildAdd(attrP, typeP);
  }

  return true;
}
#endif



// -----------------------------------------------------------------------------
//
// attributeTransform -
//
static inline void attributeTransform(KjNode* attrP, const char* type, KjNode* valueP)
{
  KjNode* typeP  = kjString(orionldState.kjsonP,  "type",  "Property");

  //
  // Transform the attribute to an Object
  //
  attrP->type              = KjObject;
  attrP->value.firstChildP = NULL;
  attrP->lastChild         = NULL;

  kjChildAdd(attrP, typeP);
  kjChildAdd(attrP, valueP);
}



// -----------------------------------------------------------------------------
//
// pbodyAttribute -
//
// With Smart-Format, an attribute RHS can be in a variety of formats:
//
// 'attrP' is the output from the parsing step. It's a tree of KjNode, the tree to amend (add missing type if need be) and check for validity.
//
// attrP->value can be:
//   * "A String"
//   * 12
//   * 3.14
//   * true/false
//   * [ 1, 2 ]
//   * {}
//
// o If 'attrP->type' is a SIMPLE FORMAT (non-array, non-object), it's quite straightforward.
//   - Just create a KjNode named "value" and make the entire 'attrP' the RHS of "value"
//     - One exception - if it's a string and the value starts with "urn:ngsi-ld", then it is assumed it's a Relationship
//   - Also, create a KjNode named "type" and set it to "Property" or "Relationship
//   - Then transform 'attrP' into an KjObject and add the "type" and "value" KjNodes to it.
//
// o If 'attrP->type' is an ARRAY, there are two possibilities:
//   - "multi-attributes" - an ARRAY of OBJECTS where at most ONE of the object may lack the datasetId member
//   - RHS of "value" and the same procedure as for SIMPLE formats is performed
//
// o If 'attrP->type' is an OBJECT:
//   - if "type" is present:
//     - and it's a valid NGSI-LD Attribute type name ("Property", "Relationship, "GeoProperty", ...)
//       then the type of the attribute is DECIDED
//     - else ("type" is not a valid type) AND "coordinates" present, and nothing else, it's considered a GeoProperty - procedure as for SIMPLE formats
//   - if "type" is NOT present:
//     - if "value/object/languageMap" is present, then we can deduct the attribute type and add it to the object. Sub-attrs are processed
//     - if no value is present, then no type is added, only sub-attrs are processed.
//
// o special attributes/sub-attributes are left untouched. They are only checked for validity
//   - Entities have the special attributes:
//       - id
//       - type
//       - scope
//     These are not processed, only checked for validity
//
//     Entities also have three special GeoProperties:
//       - location
//       - observationSpace
//       - operationSpace
//     These are both processed and checked for validity - must be GeoProperty!
//
//  - Attributes of type Property can have the following special attributes:
//      - type
//      - value
//      - observedAt
//      - unitCode
//      - datasetId (sub-attributes don't have datasetId)
//
//  - Attributes of type GeoProperty can have the following special attributes:
//      - type
//      - value
//      - observedAt
//      - datasetId (sub-attributes don't have datasetId)
//
//  - Attributes of type Relationship can have the following special attributes:
//      - type
//      - object
//      - observedAt
//      - datasetId (sub-attributes don't have datasetId)
//
//  - Attributes of type LanguageProperty can have the following special attributes:
//      - type
//      - languageMap
//      - observedAt
//      - datasetId (sub-attributes don't have datasetId)
//
bool pbodyAttribute(KjNode* attrP, int nestingLevel, OrionldProblemDetails* pdP)
{
  LM_TMP(("KZ: RHS of attribute '%s' is of JSON type %s", attrP->name, kjValueType(attrP->type)));

  if      (strcmp(attrP->name, "type")        == 0) return true;
  else if (strcmp(attrP->name, "value")       == 0) return true;
  else if (strcmp(attrP->name, "object")      == 0) return true;
  else if (strcmp(attrP->name, "languageMap") == 0) return true;
  else if (strcmp(attrP->name, "unitCode")    == 0) return true;
  else if (strcmp(attrP->name, "datasetId")   == 0) return true;
  else if (strcmp(attrP->name, "observedAt")  == 0) return true;
  else if (strcmp(attrP->name, "@context")    == 0) return true;

  LM_TMP(("KZ: '%s' seems like a 'normal' attribute (RHS is of type '%s'). Let's dive in!", attrP->name, kjValueType(attrP->type)));
  if (attrP->type == KjArray)
  {
    // ToDo: Check for datasetId !!!
    KjNode* valueP = kjArray(orionldState.kjsonP,  "value");

    valueP->value.firstChildP = attrP->value.firstChildP;
    valueP->lastChild         = attrP->lastChild;

    attributeTransform(attrP, "Property", valueP);
  }
  else if (attrP->type == KjInt)
  {
    KjNode* valueP = kjInteger(orionldState.kjsonP, "value", attrP->value.i);
    attributeTransform(attrP, "Property", valueP);
  }
  else if (attrP->type == KjString)
  {
    bool     relationship = (strncmp(attrP->value.s, "urn:ngsi-ld:", 12) == 0);
    char*    valueKey;
    char*    attrType;
    KjNode*  valueP;

    if (relationship == false)
    {
      valueKey = (char*) "value";
      attrType = (char*) "Property";
    }
    else
    {
      valueKey = (char*) "object";
      attrType = (char*) "Relationship";
    }

    valueP = kjString(orionldState.kjsonP, valueKey, attrP->value.s);

    attributeTransform(attrP, attrType, valueP);
  }
  else if (attrP->type == KjFloat)
  {
    KjNode* valueP = kjFloat(orionldState.kjsonP,  "value", attrP->value.f);
    attributeTransform(attrP, "Property", valueP);
  }
  else if (attrP->type == KjBoolean)
  {
    KjNode* valueP = kjBoolean(orionldState.kjsonP,  "value", attrP->value.b);
    attributeTransform(attrP, "Property", valueP);
  }
  else if (attrP->type == KjObject)
  {
    if (nestingLevel == 0)
    {
      //
      // Might be the "value" of a GeoProperty ( type + coordinates must be present )
      // Might be the "value" of a LanguageProperty  ( all fields are strings ... a bit risky ...)
      // Normally it's the RHS of an attribute - { "type": XX, "value": XX, ... }
      // If RHS, we must loop over all fields and check + fill in missing fields
      //
      KjNode* typeP = kjLookup(attrP, "type");

      if ((typeP != NULL) && (kjLookup(attrP, "coordinates") != NULL))
      {
        LM_TMP(("KZ: '%s' seems like a 'GeoProperty' VALUE", attrP->name));
        // Seems like the value of a GeoProperty
        if (pcheckGeoPropertyValue(attrP, pdP) == true)
        {
          KjNode* typeP  = kjString(orionldState.kjsonP, "type",  "GeoProperty");
          KjNode* valueP = kjObject(orionldState.kjsonP,  "value");

          valueP->value.firstChildP = attrP->value.firstChildP;
          valueP->lastChild         = attrP->lastChild;

          attrP->value.firstChildP = NULL;
          attrP->lastChild         = NULL;

          kjChildAdd(attrP, typeP);
          kjChildAdd(attrP, valueP);

          return true;
        }
      }

      //
      // Check for LanguageProperty value
      // All fields of the Object must be strings
      // All keys must be valid shortnames for languages, like 'en', 'es', 'sw', ...
      //
      if (typeP == NULL)
      {
        bool notLanguageProperty = false;
        for (KjNode* fieldP = attrP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next)
        {
          LM_TMP(("KZ: Checking field '%s' for String", fieldP->name));
          if (fieldP->type != KjString)
          {
            notLanguageProperty = true;
            break;
          }
        }

        if ((notLanguageProperty == false) && (attrP->value.firstChildP != NULL))
        {
          LM_TMP(("KZ: '%s' is understood as a 'LanguageProperty' VALUE", attrP->name));

          KjNode* typeP  = kjString(orionldState.kjsonP, "type",  "LanguageProperty");
          KjNode* valueP = kjObject(orionldState.kjsonP, "languageMap");

          valueP->value.firstChildP = attrP->value.firstChildP;
          valueP->lastChild         = attrP->lastChild;

          kjChildAdd(attrP, typeP);
          kjChildAdd(attrP, valueP);

          return true;
        }
      }
    }

    // So, not any kind of Property value (not Geo, not Language)
    // Then it's the RHS of attrP - we need a "value", an "object", or a "languageMap" field to be present.
    // If none of the three are found, then it's a 400 Bad Request.
    // If the type of rthe attribute is not present, we add it
    //
    LM_TMP(("KZ: The RHS of '%s' is a normal RHS for an attribute - we need a value, an object, or a languageMap", attrP->name));
    KjNode*  valueP        = kjLookup(attrP, "value");
    KjNode*  objectP       = kjLookup(attrP, "object");
    KjNode*  languageMapP  = kjLookup(attrP, "languageMap");

    if ((valueP == NULL) && (objectP == NULL) && (languageMapP == NULL))
    {
      LM_W(("RHS for attribute '%s' is an Object. Not a GeoProp, nor a LanguageProp and no value/object/languageMap is present"));
      orionldError(pdP,
                   OrionldBadRequestData,
                   "Mandatory field missing",
                   "value/object/languageMap",
                   400);
      return false;
    }

    //
    // Loop over all members of the object
    //
    ++nestingLevel;
    for (KjNode* fieldP = attrP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next)
    {
      pbodyAttribute(fieldP, nestingLevel, pdP);
    }
  }
  else if (attrP->type == KjNull)
  {
    orionldError(pdP,
                 OrionldBadRequestData,
                 "The use NULL values is banned in NGSI-LD",
                 attrP->name,
                 400);
      return false;
  }

  return true;
}
