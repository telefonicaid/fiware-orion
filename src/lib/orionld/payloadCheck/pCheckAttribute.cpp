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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/types/OrionldResponseErrorType.h"              // OrionldBadRequestData
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, orionldAttributeTypeName
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/payloadCheck/pCheckAttribute.h"                // Own interface



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
  OrionldAttributeType*   attributeTypeP
)
{
  if (typeP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Invalid attribute type field", "Must be a String", 400);
    return false;
  }

  if      (strcmp(typeP->value.s, "Property")          == 0) *attributeTypeP = Property;
  else if (strcmp(typeP->value.s, "Relationship")      == 0) *attributeTypeP = Relationship;
  else if (strcmp(typeP->value.s, "GeoProperty")       == 0) *attributeTypeP = GeoProperty;
  else if (strcmp(typeP->value.s, "LanguageProperty")  == 0) *attributeTypeP = LanguageProperty;
  else
  {
    orionldError(OrionldBadRequestData, "Invalid value for attribute type field", typeP->value.s, 400);
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
  OrionldAttributeType*   attributeTypeP
)
{
  if ((typeP != NULL) && (pcheckAttributeTypeValue(typeP, attributeTypeP) == false))
    return false;

  if (*attributeTypeP == Property)
  {
    if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A Property cannot have an /object/ field",
                   400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData,
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
      orionldError(OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A GeoProperty cannot have an /object/ field",
                   400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData,
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
      orionldError(OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A Relationship cannot have a /value/ field",
                   400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData,
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
      orionldError(OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A LanguageMap cannot have a /value/ field",
                   400);
      return false;
    }
    else if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData,
                   "Invalid combination of attribute type and value",
                   "A LanguageMap cannot have an /object/ field",
                   400);
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// attributeTransform -
//
static inline void attributeTransform(KjNode* attrP, const char* type, KjNode* valueP)
{
  KjNode* typeP = kjString(orionldState.kjsonP,  "type",  type);

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
// pCheckAttributeString -
//
inline bool pCheckAttributeString(KjNode* attrP, bool isAttribute, OrionldAttributeType attrTypeFromDb)
{
  bool     relationship = (strncmp(attrP->value.s, "urn:ngsi-ld:", 12) == 0);
  char*    valueKey;
  char*    attrType;
  KjNode*  valueP;

  if (attrTypeFromDb == Property)
  {
    // Even if the string is a URI, if the attribute already exists and its a Property, all good
  }
  else if (attrTypeFromDb == NoAttributeType)
  {
    // If the attribute did not exist, also no problem
    // I think ... :)
    // This might depend on the SERVICE and if attr or sub-attr
  }
  else if ((relationship == true) && (attrTypeFromDb != Relationship))
  {
    LM_TMP(("attrTypeFromDb: %d", attrTypeFromDb));
    orionldError(OrionldBadRequestData, "Attempt to transform a GeoProperty into a Relationship", attrP->name, 400);
    return false;
  }
  else if (relationship == false)
  {
    if (attrTypeFromDb == Relationship)
      orionldError(OrionldBadRequestData, "Attempt to transform a Relationship into a Property", attrP->name, 400);
    else if (attrTypeFromDb == GeoProperty)
      orionldError(OrionldBadRequestData, "Attempt to transform a GeoProperty into a Property", attrP->name, 400);

    return false;
  }


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
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeInteger -
//
inline bool pCheckAttributeInteger(KjNode* attrP, bool isAttribute, OrionldAttributeType attrTypeFromDb)
{
  if ((attrTypeFromDb != NoAttributeType) && (attrTypeFromDb != Property))
  {
    if (attrTypeFromDb == Relationship)
      orionldError(OrionldBadRequestData, "Attempt to transform a Relationship into a Property", attrP->name, 400);
    else if (attrTypeFromDb == GeoProperty)
      orionldError(OrionldBadRequestData, "Attempt to transform a GeoProperty into a Property", attrP->name, 400);

    return false;
  }

  KjNode* valueP = kjInteger(orionldState.kjsonP, "value", attrP->value.i);
  attributeTransform(attrP, "Property", valueP);
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeFloat -
//
inline bool pCheckAttributeFloat(KjNode* attrP, bool isAttribute, OrionldAttributeType attrTypeFromDb)
{
  if ((attrTypeFromDb != NoAttributeType) && (attrTypeFromDb != Property))
  {
    if (attrTypeFromDb == Relationship)
      orionldError(OrionldBadRequestData, "Attempt to transform a Relationship into a Property", attrP->name, 400);
    else if (attrTypeFromDb == GeoProperty)
      orionldError(OrionldBadRequestData, "Attempt to transform a GeoProperty into a Property", attrP->name, 400);

    return false;
  }

  KjNode* valueP = kjFloat(orionldState.kjsonP,  "value", attrP->value.f);
  attributeTransform(attrP, "Property", valueP);
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeBoolean -
//
inline bool pCheckAttributeBoolean(KjNode* attrP, bool isAttribute, OrionldAttributeType attrTypeFromDb)
{
  if ((attrTypeFromDb != NoAttributeType) && (attrTypeFromDb != Property))
  {
    if (attrTypeFromDb == Relationship)
      orionldError(OrionldBadRequestData, "Attempt to transform a Relationship into a Property", attrP->name, 400);
    else if (attrTypeFromDb == GeoProperty)
      orionldError(OrionldBadRequestData, "Attempt to transform a GeoProperty into a Property", attrP->name, 400);

    return false;
  }

  KjNode* valueP = kjBoolean(orionldState.kjsonP,  "value", attrP->value.b);
  attributeTransform(attrP, "Property", valueP);
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeArray -
//
inline bool pCheckAttributeArray(KjNode* attrP, bool isAttribute, OrionldAttributeType attrTypeFromDb)
{
  // ToDo: Check for datasetId !!!
  KjNode* valueP = kjArray(orionldState.kjsonP,  "value");

  valueP->value.firstChildP = attrP->value.firstChildP;
  valueP->lastChild         = attrP->lastChild;

  attributeTransform(attrP, "Property", valueP);

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeNull -
//
inline bool pCheckAttributeNull(KjNode* attrP)
{
  LM_W(("RHS for attribute '%s' is NULL - that is forbidden in the NGSI-LD API", attrP->name));
  orionldError(OrionldBadRequestData,
               "The use of NULL value is banned in NGSI-LD",
               attrP->name,
               400);
  return false;
}



// -----------------------------------------------------------------------------
//
// pCheckGeoPropertyValue - move!
//
bool pCheckGeoPropertyValue(KjNode* attrP)
{
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeType - move!
//
bool pCheckAttributeType(const char* typeValue)
{
  // LanguageProperty, TemporalProperty, VocabProperty ...

  return false;
}



// -----------------------------------------------------------------------------
//
// pCheckGeoPropertyValue -
//
bool pCheckGeoPropertyValue(KjNode* attrP, KjNode* typeP)
{
  LM_TMP(("KZ: Looking up coordinates"));
  KjNode* coordinatesP = kjLookup(attrP, "coordinates");

  if (coordinatesP != NULL)
  {
    LM_TMP(("KZ: Found coordinates"));
    if (pCheckGeoPropertyValue(attrP) == true)
    {
      // Convert to Normalized
      KjNode* valueP = kjObject(orionldState.kjsonP,  "value");

      valueP->value.firstChildP = attrP->value.firstChildP;
      valueP->lastChild         = attrP->lastChild;

      LM_TMP(("Transforming attr into a GeoProperty"));
      attributeTransform(attrP, "GeoProperty", valueP);
      LM_TMP(("KZ: All good"));
      return true;
    }
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// typeCheck -
//
bool pCheckAttributeType(KjNode* attrP, KjNode** typePP, bool mandatory)
{
  KjNode* typeP   = kjLookup(attrP, "type");
  KjNode* atTypeP = kjLookup(attrP, "@type");

  //
  // It's allowed to use either "type" or "@type" but not both
  //
  if ((typeP != NULL) && (atTypeP != NULL))
  {
    orionldError(OrionldBadRequestData, "Duplicate Fields in payload body", "type and @type", 400);
    return false;
  }

  //
  // If "type" is absent, perhaps "@type" is present?
  //
  if (typeP == NULL)
    typeP = atTypeP;

  //
  // If "type" is mandatory but absent - error
  //
  if ((mandatory == true) && (typeP == NULL))
  {
    orionldError(OrionldBadRequestData, "Missing /type/ field for an attribute", attrP->name, 400);
    return false;
  }

  //
  // The type field MUST be a string
  //
  if ((typeP != NULL) && (typeP->type != KjString))
  {
    orionldError(OrionldBadRequestData, "Invalid JSON type for /type/ member", kjValueType(typeP->type), 400);
    return false;
  }

  //
  // Save the pointer to the type for later use
  //
  *typePP = typeP;

  return true;
}



// -----------------------------------------------------------------------------
//
// valueAndTypeCheck -
//
bool valueAndTypeCheck(KjNode* attrP, OrionldAttributeType attributeType, KjNode** valuePP)
{
  KjNode* valueP       = kjLookup(attrP, "value");
  KjNode* objectP      = kjLookup(attrP, "object");
  KjNode* languageMapP = kjLookup(attrP, "languageMap");

  if (attributeType == Property)
  {
    if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData, "Unsupported field", "object", 400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData, "Unsupported field", "languageMap", 400);
      return false;
    }
  }
  else if (attributeType == Relationship)
  {
    if (valueP != NULL)
    {
      orionldError(OrionldBadRequestData, "Unsupported field", "value", 400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData, "Unsupported field", "languageMap", 400);
      return false;
    }
  }
  else if (attributeType == LanguageProperty)
  {
    if (valueP != NULL)
    {
      orionldError(OrionldBadRequestData, "Unsupported field", "value", 400);
      return false;
    }
    else if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData, "Unsupported field", "object", 400);
      return false;
    }
  }

  return true;
}



bool unitCodeCheck(KjNode* fieldP) { return true; }
bool datasetIdCheck(KjNode* fieldP) { return true; }
bool timestampCheck(KjNode* fieldP) { return true; }
bool objectCheck(KjNode* fieldP) { return true; }
bool languageMapCheck(KjNode* fieldP) { return true; }


// -----------------------------------------------------------------------------
//
// pCheckAttributeObject -
//
// - if "type" is present inside the object:
//   - and it's a valid NGSI-LD Attribute type name ("Property", "Relationship, "GeoProperty", ...)
//     then the type of the attribute is DECIDED
//   - else ("type" is not a valid type) AND "coordinates" present, and nothing else,
//     it's considered a GeoProperty - procedure as for SIMPLE formats
// - if "type" is NOT present:
//   - if "value/object/languageMap" is present, then we can deduct the attribute type and add it to the object. Sub-attrs are processed
//   - if no value is present, then no type is added, only sub-attrs are processed.
//
bool pCheckAttributeObject
(
  KjNode*                 attrP,
  bool                    isAttribute,
  KjNode*                 dbAttributeP,
  OrionldAttributeType    attrTypeFromDb
)
{
  OrionldAttributeType  attributeType = NoAttributeType;
  KjNode*               typeP;
  KjNode*               valueP = NULL;  // "object" if Relationship, "languageMap" if LanguageProperty

  // Check for errors in the input payload for the attribute type
  if (pCheckAttributeType(attrP, &typeP, false) == false)
    return false;

  // If the attribute already exists, we KMOW the type of the attribute.
  // If the payload body contains a type, and it's not the same type, well, that's an error right there
  //
  // However, the payload body *might* be the value of a GeoProperty.
  // If so, the payload body would have a type == "Point", "Polygon" or something similar.
  // We CAN'T compare the type with what "should be according to the DB" until we rule GeoProperty value out.
  //
  // That is done a little later in this function - see "All good, let's now compare with the truth"
  //

  //
  // Attribute Type given.
  // 1. If the attribute exists already, is the "new" type the same (forbidden to change attribute types
  // 2. ...
  //
  if (typeP != NULL)
  {
    //
    // OK, type given in the payload body.
    // If the entity/attribute already existed, we can compare. Attribute types MAY NOT BE ALTERED
    //
    LM_TMP(("KZ: attribute type: %s", typeP->value.s));
    attributeType = orionldAttributeType(typeP->value.s);

    if (attributeType == NoAttributeType)  // Invalid attribute type, but, it might still be the value of a GeoProperty
    {
      if (pCheckGeoPropertyValue(attrP, typeP) == false)
      {
        // Not a GeoProperty, so, invalid attribute type
        orionldError(OrionldBadRequestData, "Invalid value for /type/ member", typeP->value.s, 400);
        return false;
      }

      return true;
    }
    else  // type is present, and has a correct value for any of { Property, GeoProperty, Relationship, ... }
    {
      LM_TMP(("KZ: I have the attribute type: '%s'", typeP->value.s));
      LM_TMP(("KZ: If I only had the attribute type from the DB also, I could compare ..."));

      // As "type" is present - is it coherent? (Property has "value", Relationship has "object", etc)
      if (valueAndTypeCheck(attrP, attributeType, &valueP) == false)
        LM_RE(false, ("valueAndTypeCheck failed"));

      //
      // All good, let's now compare with the truth - input attr type VS what's in the DB
      //
      if (attrTypeFromDb != NoAttributeType)  // Means it already existed, means we can compare
      {
        LM_TMP(("KZ: Comparing new/old attribute types: NEW '%d' vs OLD '%d'", attributeType, attrTypeFromDb));
        if (attributeType != attrTypeFromDb)
        {
          LM_E(("Bad Input (attempt to change the Attribute Type of '%s' from '%d' to '%d'", attrP->name, attrTypeFromDb, attributeType));
          orionldError(OrionldBadRequestData, "Attempt to change the Type of an Attribute", attrP->name, 400);
          return false;
        }
      }
    }
  }
  else  // type is not there - try to guess the type, if not already known from the DB
  {
    if (attrTypeFromDb != NoAttributeType)
      attributeType = attrTypeFromDb;
    else
    {
      KjNode* objectP      = kjLookup(attrP, "object");
      KjNode* languageMapP = kjLookup(attrP, "languageMap");

      if ((valueP = kjLookup(attrP, "value")) != NULL)
      {
        LM_TMP(("KZ: found a 'value', assuming Property"));
        attributeType = Property;  // Well ... or GeoProperty!
      }
      else if (objectP != NULL)
        attributeType = Relationship;
      else if (languageMapP != NULL)
        attributeType = LanguageProperty;
    }
  }
  LM_TMP(("attributeType: %d", attributeType));

  KjNode* fieldP = attrP->value.firstChildP;
  KjNode* next;

  while (fieldP != NULL)
  {
    next = fieldP->next;

    if ((fieldP == typeP) || (fieldP == valueP))
    {
      // The value/object/languageMap is left as is
    }
    else if (strcmp(fieldP->name, "value") == 0)
    {
      if ((attributeType == Relationship) || (attributeType == LanguageProperty))
      {
        orionldError(OrionldBadRequestData, "Invalid member /value/", "valid for Property/GeoProperty attributes only", 400);
        return false;
      }
    }
    else if (strcmp(fieldP->name, "observedAt") == 0)
    {
      if (timestampCheck(fieldP) == false)
        return false;
    }
    else if ((isAttribute == true) && (strcmp(fieldP->name, "datasetId") == 0))
    {
      if (datasetIdCheck(fieldP) == false)
        return false;
    }
    else if (strcmp(fieldP->name, "unitCode") == 0)
    {
      if ((attributeType == Property) || (attributeType == NoAttributeType))
      {
        LM_TMP(("KZ: unitCode for attribute of type: %d", attributeType));
        if (unitCodeCheck(fieldP) == false)
          return false;
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid member /unitCode/", "valid for Property attributes only", 400);
        return false;
      }
    }
    else if (strcmp(fieldP->name, "object") == 0)
    {
      if (attributeType == Relationship)
      {
        LM_TMP(("Calling objectCheck for a Relationship"));
        if (objectCheck(fieldP) == false)
        return false;
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid member /object/", "valid for Relationship attributes only", 400);
        return false;
      }
    }
    else if ((attributeType == LanguageProperty) && (strcmp(fieldP->name, "languageMap") == 0))
    {
      if (languageMapCheck(fieldP) == false)
        return false;
    }
    else
    {
      //
      // To make sure the attribute type of the sub-attribute is not modified,
      // we need to look up the sub-attr from the database and extract the attribute type from there.
      // This info is then input to pCheckAttribute.
      // If this is the creation of an attribute, its dbAttributeP will be NULL and there is no extra info to achieve.
      //
      OrionldAttributeType  subAttributeType = NoAttributeType;
      KjNode*               dbSubAttrP       = NULL;

      if (dbAttributeP != NULL)
      {
        char buf[2048];
        kjFastRender(dbAttributeP, buf);
        LM_TMP(("dbAttribute: %s", buf));

        const char*           dbField          = (isAttribute == true)? "attrs" : "md";
        KjNode*               mdP              = kjLookup(dbAttributeP, dbField);

        if (mdP)
        {
          char*   longName   = orionldContextItemExpand(orionldState.contextP, fieldP->name, true, NULL);
          char*   longNameEq = kaStrdup(&orionldState.kalloc, longName);

          if (longNameEq != NULL)
          {
            dotForEq(longNameEq);
            dbSubAttrP = kjLookup(mdP, longNameEq);

            if (dbSubAttrP != NULL)
            {
              KjNode* typeP = kjLookup(dbSubAttrP, "type");

              if (typeP != NULL)
                subAttributeType = orionldAttributeType(typeP->value.s);
            }
          }
        }
      }

      if (pCheckAttribute(fieldP, false, dbSubAttrP, subAttributeType) == false)  // Need to find fieldP->name in dbAttributeP ...
        return false;
    }

    fieldP = next;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttribute -
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
// Or, as source code:
//   if (attrP->type == KjArray)
//     pCheckArrayAttribute(attrP)
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
bool pCheckAttribute
(
  KjNode*                 attrP,
  bool                    isAttribute,
  KjNode*                 dbAttributeP,
  OrionldAttributeType    attrTypeFromDb
)
{
  KjNode* typeP = kjLookup(attrP, "type");
  if (typeP)
    LM_TMP(("KZ: RHS of %s attribute '%s' is a JSON %s", typeP->name, attrP->name, kjValueType(attrP->type)));
  else
    LM_TMP(("KZ: RHS of attribute '%s' is a JSON %s", attrP->name, kjValueType(attrP->type)));

  if      (attrP->type == KjString)  return pCheckAttributeString(attrP,  isAttribute, attrTypeFromDb);
  else if (attrP->type == KjInt)     return pCheckAttributeInteger(attrP, isAttribute, attrTypeFromDb);
  else if (attrP->type == KjFloat)   return pCheckAttributeFloat(attrP,   isAttribute, attrTypeFromDb);
  else if (attrP->type == KjBoolean) return pCheckAttributeBoolean(attrP, isAttribute, attrTypeFromDb);
  else if (attrP->type == KjArray)   return pCheckAttributeArray(attrP,   isAttribute, attrTypeFromDb);
  else if (attrP->type == KjObject)  return pCheckAttributeObject(attrP,  isAttribute, dbAttributeP, attrTypeFromDb);
  else if (attrP->type == KjNull)    return pCheckAttributeNull(attrP);

  // Invalid JSON type of the attribute
  LM_W(("Unknown JSON type for the Right-Hand-Side of the attribute '%s'", attrP->name));
  orionldError(OrionldInternalError,
               "invalid value type for attribute",
               attrP->name,
               500);
  return false;
}
