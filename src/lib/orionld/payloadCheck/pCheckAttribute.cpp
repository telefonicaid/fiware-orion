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
#include <unistd.h>                                              // NULL

extern "C"
{
#include "kbase/kMacros.h"                                       // K_FT
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/types/OrionldResponseErrorType.h"              // OrionldBadRequestData
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, orionldAttributeTypeName
#include "orionld/types/OrionldGeometry.h"                       // orionldGeometryFromString, GeoNoGeometry
#include "orionld/types/OrionLdRestService.h"                    // ORIONLD_SERVICE_OPTION_ACCEPT_JSONLD_NULL
#include "orionld/types/OrionldContextItem.h"                    // OrionldContextItem
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dateTime.h"                             // dateTimeFromString
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/context/orionldSubAttributeExpand.h"           // orionldSubAttributeExpand
#include "orionld/serviceRoutines/orionldPatchEntity2.h"         // orionldPatchEntity2
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/payloadCheck/pcheckName.h"                     // pCheckName
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/payloadCheck/pCheckAttributeTransform.h"       // pCheckAttributeTransform
#include "orionld/payloadCheck/pCheckGeoProperty.h"              // pCheckGeoProperty
#include "orionld/payloadCheck/pCheckAttributeType.h"            // pCheckAttributeType
#include "orionld/payloadCheck/pCheckAttribute.h"                // Own interface



// -----------------------------------------------------------------------------
//
// attrTypeChangeTitle -
//
static const char* attrTypeChangeTitle(OrionldAttributeType oldType, OrionldAttributeType newType)
{
  if (newType == Property)
  {
    if (oldType == Relationship)        return "Attempt to transform a Relationship into a Property";
    if (oldType == GeoProperty)         return "Attempt to transform a GeoProperty into a Property";
    if (oldType == LanguageProperty)    return "Attempt to transform a LanguageProperty into a Property";
    if (oldType == VocabularyProperty)  return "Attempt to transform a VocabularyProperty into a Property";
    if (oldType == JsonProperty)        return "Attempt to transform a JsonProperty into a Property";
  }
  else if (newType == Relationship)
  {
    if (oldType == Property)            return "Attempt to transform a Property into a Relationship";
    if (oldType == GeoProperty)         return "Attempt to transform a GeoProperty into a Relationship";
    if (oldType == LanguageProperty)    return "Attempt to transform a LanguageProperty into a Relationship";
    if (oldType == VocabularyProperty)  return "Attempt to transform a VocabularyProperty into a Relationship";
    if (oldType == JsonProperty)        return "Attempt to transform a JsonProperty into a Relationship";
  }
  else if (newType == GeoProperty)
  {
    if (oldType == Property)            return "Attempt to transform a Property into a GeoProperty";
    if (oldType == Relationship)        return "Attempt to transform a Relationship into a GeoProperty";
    if (oldType == LanguageProperty)    return "Attempt to transform a LanguageProperty into a GeoProperty";
    if (oldType == VocabularyProperty)  return "Attempt to transform a VocabularyProperty into a GeoProperty";
    if (oldType == JsonProperty)        return "Attempt to transform a JsonProperty into a GeoProperty";
  }
  else if (newType == LanguageProperty)
  {
    if (oldType == Property)            return "Attempt to transform a Property into a LanguageProperty";
    if (oldType == Relationship)        return "Attempt to transform a Relationship into a LanguageProperty";
    if (oldType == GeoProperty)         return "Attempt to transform a GeoProperty into a LanguageProperty";
    if (oldType == VocabularyProperty)  return "Attempt to transform a VocabularyProperty into a LanguageProperty";
    if (oldType == JsonProperty)        return "Attempt to transform a JsonProperty into a LanguageProperty";
  }
  else if (newType == VocabularyProperty)
  {
    if (oldType == Property)            return "Attempt to transform a Property into a VocabularyProperty";
    if (oldType == Relationship)        return "Attempt to transform a Relationship into a VocabularyProperty";
    if (oldType == GeoProperty)         return "Attempt to transform a GeoProperty into a VocabularyProperty";
    if (oldType == LanguageProperty)    return "Attempt to transform a LanguageProperty into a VocabularyProperty";
    if (oldType == JsonProperty)        return "Attempt to transform a JsonProperty into a VocabularyProperty";
  }
  else if (newType == JsonProperty)
  {
    if (oldType == Property)            return "Attempt to transform a Property into a JsonProperty";
    if (oldType == Relationship)        return "Attempt to transform a Relationship into a JsonProperty";
    if (oldType == GeoProperty)         return "Attempt to transform a GeoProperty into a JsonProperty";
    if (oldType == LanguageProperty)    return "Attempt to transform a LanguageProperty into a JsonProperty";
    if (oldType == VocabularyProperty)  return "Attempt to transform a VocabularyProperty into a JsonProperty";
  }

  return "Attribute type inconsistency";
}



// -----------------------------------------------------------------------------
//
// arrayReduce -
//
static void arrayReduce(KjNode* valueP)
{
  if (valueP->type != KjArray)
    return;

  if (valueP->value.firstChildP == NULL)
    return;

  if (valueP->value.firstChildP->next != NULL)
    return;

  // It's an array with one single item
  KjNode* itemP = valueP->value.firstChildP;

  valueP->type      = itemP->type;
  valueP->value     = itemP->value;
  valueP->lastChild = itemP->lastChild;

  LM_T(LmtArrayReduction, ("Reduced an array of one single item to a JSON %s", kjValueType(valueP->type)));
}



// -----------------------------------------------------------------------------
//
// pCheckTypeFromContext -
//
static bool pCheckTypeFromContext(KjNode* attrP, OrionldContextItem* attrContextInfoP)
{
  bool arrayReduction = true;

  if ((attrContextInfoP != NULL) && (attrContextInfoP->type != NULL))
  {
    if (strcmp(attrContextInfoP->type, "DateTime") == 0)
    {
      if (attrP->type != KjString)
      {
        orionldError(OrionldBadRequestData,
                     "JSON Type for attribute not according to @context @type field",
                     attrP->name,
                     400);
        return false;
      }

      char errorString[256];
      if (dateTimeFromString(attrP->value.s, errorString, sizeof(errorString)) < 0)
      {
        //
        // Deletion?
        // Should only be valid for merge+patch ops ...
        //
        if (strcmp(attrP->value.s, "urn:ngsi-ld:null") == 0)
        {
          attrP->type = KjNull;
          return true;
        }

        orionldError(OrionldBadRequestData, "Invalid ISO8601 timestamp", errorString, 400);
        pdField(attrP->name);

        return false;
      }
    }
    else if (strcmp(attrContextInfoP->type, "@id") == 0)
    {
      if (attrP->type != KjString)
      {
        orionldError(OrionldBadRequestData,
                     "JSON Type for attribute not according to @context @type field",
                     attrP->name,
                     400);
        return false;
      }

      if (pCheckUri(attrP->value.s, attrP->name, true) == false)
      {
        orionldError(OrionldBadRequestData,
                     "Not a valid URI",
                     attrP->name,
                     400);
        return false;
      }
    }
    else if (strcmp(attrContextInfoP->type, "@set") == 0)
    {
      LM_T(LmtArrayReduction, ("the type for '%s' in the @context is '@set' - arrayReduction is set to FALSE", attrP->name));
      arrayReduction = false;
    }
    else if (strcmp(attrContextInfoP->type, "@list") == 0)
    {
      LM_T(LmtArrayReduction, ("the type for '%s' in the @context is '@list' - arrayReduction is set to FALSE", attrP->name));
      arrayReduction = false;
    }
  }

  //
  // Array Reduction
  //
  LM_T(LmtArrayReduction, ("Attribute: '%s' - arrayReduction: %s, type '%s'", attrP->name, K_FT(arrayReduction), kjValueType(attrP->type)));

  if ((attrP->type == KjArray) && (arrayReduction == true))
  {
    LM_T(LmtArrayReduction, ("The value of '%s' is an array and arrayReduction is ON", attrP->name));
    if ((attrP->value.firstChildP != NULL) && (attrP->value.firstChildP->next == NULL))
    {
      LM_T(LmtArrayReduction, ("'%s' is an array of one single element => arrayReduction is PERFORMED", attrP->name));

      KjNode* arrayItemP = attrP->value.firstChildP;

      attrP->type      = arrayItemP->type;
      attrP->value     = arrayItemP->value;
      attrP->lastChild = arrayItemP->lastChild;  // Might be an array or object inside the array ...
    }
    else
      LM_T(LmtArrayReduction, ("'%s' is an array of zero od +1 elements => arrayReduction is NOT performed", attrP->name));
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeString -
//
// A String is always a Property.
// Make sure that:
// - if the attribute already existed, that it is a Property in the DB
// - OR: it's a new attribute
// - OR, the string is "urn:ngsi-ld:null" ... (then it could also be a Relationship - doesn't matter)
//
inline bool pCheckAttributeString
(
  const char*           entityId,
  KjNode*               attrP,
  bool                  isAttribute,
  OrionldAttributeType  attrTypeFromDb,
  OrionldContextItem*   attrContextInfoP
)
{
  PCHECK_SPECIAL_ATTRIBUTE(attrP, isAttribute, attrTypeFromDb);

  // Special case:
  //   options=keyValues is set
  //   The service routine is orionldPatchEntity2
  //   The attribute already exists in the DB
  //   The attribute is a Relationship in the DB
  //   The new value is a JSON String, that is a valid URI
  //
  // If all those are fulfilled, then the value (object) of the Relationship will be modified
  //
  if ((orionldState.out.format == RF_SIMPLIFIED)                      &&
      (orionldState.serviceP->serviceRoutine  == orionldPatchEntity2) &&
      (attrTypeFromDb                         == Relationship))
  {
    if (pCheckUri(attrP->value.s, attrP->name, true) == false)
    {
      orionldError(OrionldBadRequestData, "PATCH of Relationship - not a valid URI", attrP->name, 400);
      return false;
    }

    KjNode* valueP = kjString(orionldState.kjsonP, "value", attrP->value.s);  // "value" or "object" ... ?
    pCheckAttributeTransform(attrP, "Relationship", valueP);
    return true;
  }

  PCHECK_NOT_A_PROPERTY(attrP, attrTypeFromDb);

  KjNode* valueP = kjString(orionldState.kjsonP, "value", attrP->value.s);
  pCheckAttributeTransform(attrP, "Property", valueP);
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeInteger -
//
inline bool pCheckAttributeInteger
(
  const char*           entityId,
  KjNode*               attrP,
  bool                  isAttribute,
  OrionldAttributeType  attrTypeFromDb,
  OrionldContextItem*   attrContextInfoP
)
{
  PCHECK_SPECIAL_ATTRIBUTE(attrP, isAttribute, attrTypeFromDb);
  PCHECK_NOT_A_PROPERTY(attrP, attrTypeFromDb);

  KjNode* valueP = kjInteger(orionldState.kjsonP, "value", attrP->value.i);
  pCheckAttributeTransform(attrP, "Property", valueP);
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeFloat -
//
inline bool pCheckAttributeFloat
(
  const char*           entityId,
  KjNode*               attrP,
  bool                  isAttribute,
  OrionldAttributeType  attrTypeFromDb,
  OrionldContextItem*   attrContextInfoP
)
{
  PCHECK_SPECIAL_ATTRIBUTE(attrP, isAttribute, attrTypeFromDb);
  PCHECK_NOT_A_PROPERTY(attrP, attrTypeFromDb);

  KjNode* valueP = kjFloat(orionldState.kjsonP,  "value", attrP->value.f);
  pCheckAttributeTransform(attrP, "Property", valueP);
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeBoolean -
//
inline bool pCheckAttributeBoolean
(
  const char*           entityId,
  KjNode*               attrP,
  bool                  isAttribute,
  OrionldAttributeType  attrTypeFromDb,
  OrionldContextItem*   attrContextInfoP
)
{
  PCHECK_SPECIAL_ATTRIBUTE(attrP, isAttribute, attrTypeFromDb);
  PCHECK_NOT_A_PROPERTY(attrP, attrTypeFromDb);

  KjNode* valueP = kjBoolean(orionldState.kjsonP,  "value", attrP->value.b);
  pCheckAttributeTransform(attrP, "Property", valueP);
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeArray -
//
inline bool pCheckAttributeArray
(
  const char*           entityId,
  KjNode*               attrP,
  bool                  isAttribute,
  OrionldAttributeType  attrTypeFromDb,
  OrionldContextItem*   attrContextInfoP
)
{
  PCHECK_SPECIAL_ATTRIBUTE(attrP, isAttribute, attrTypeFromDb);
  PCHECK_NOT_A_PROPERTY(attrP, attrTypeFromDb);

  KjNode* valueP = kjArray(orionldState.kjsonP,  "value");

  valueP->value.firstChildP = attrP->value.firstChildP;
  valueP->lastChild         = attrP->lastChild;

  pCheckAttributeTransform(attrP, "Property", valueP);
  arrayReduce(valueP);
  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttributeNull -
//
inline bool pCheckAttributeNull(const char* entityId, KjNode* attrP)
{
#if 0
  LM_W(("RHS for attribute '%s' is NULL - that is forbidden in the NGSI-LD API", attrP->name));
  orionldError(OrionldBadRequestData,
               "The use of NULL value is banned in NGSI-LD",
               attrP->name,
               400);
  return false;
#else
  return true;
#endif
}



// -----------------------------------------------------------------------------
//
// valueAndTypeCheck -
//
bool valueAndTypeCheck(KjNode* attrP, OrionldAttributeType attributeType, bool attributeExisted)
{
  KjNode* valueP       = kjLookup(attrP, "value");
  KjNode* objectP      = kjLookup(attrP, "object");
  KjNode* languageMapP = kjLookup(attrP, "languageMap");
  KjNode* vocabP       = kjLookup(attrP, "vocab");
  KjNode* jsonP        = kjLookup(attrP, "json");

  if (attributeType == Property)
  {
    if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a Property: object", attrP->name, 400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a Property: languageMap", attrP->name, 400);
      return false;
    }
    else if (vocabP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a Property: vocab", attrP->name, 400);
      return false;
    }
    else if (jsonP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a Property: json", attrP->name, 400);
      return false;
    }
    else if ((valueP == NULL) && (attributeExisted == false))  // Attribute is new but the value is missing
    {
      orionldError(OrionldBadRequestData, "Missing /value/ field for Property at creation time", attrP->name, 400);
      return false;
    }
  }
  if (attributeType == GeoProperty)
  {
    if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a GeoProperty: object", attrP->name, 400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a GeoProperty: languageMap", attrP->name, 400);
      return false;
    }
    else if (vocabP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a GeoProperty: vocab", attrP->name, 400);
      return false;
    }
    else if (jsonP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a GeoProperty: json", attrP->name, 400);
      return false;
    }
    else if ((valueP == NULL) && (attributeExisted == false))  // Attribute is new but the value is missing
    {
      orionldError(OrionldBadRequestData, "Missing /value/ field for GeoProperty at creation time", attrP->name, 400);
      return false;
    }
  }
  else if (attributeType == Relationship)
  {
    if (valueP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a Relationship: value", attrP->name, 400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a Relationship: languageMap", attrP->name, 400);
      return false;
    }
    else if (vocabP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a Relationship: vocab", attrP->name, 400);
      return false;
    }
    else if (jsonP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a Relationship: json", attrP->name, 400);
      return false;
    }
    else if ((objectP == NULL) && (attributeExisted == false))  // Attribute is new but the value is missing
    {
      orionldError(OrionldBadRequestData, "Missing /object/ field for Relationship at creation time", attrP->name, 400);
      return false;
    }
  }
  else if (attributeType == LanguageProperty)
  {
    if (valueP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a LanguageProperty: value", attrP->name, 400);
      return false;
    }
    else if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a LanguageProperty: object", attrP->name, 400);
      return false;
    }
    else if (vocabP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a LanguageProperty: vocab", attrP->name, 400);
      return false;
    }
    else if (jsonP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a LanguageProperty: json", attrP->name, 400);
      return false;
    }
    else if ((languageMapP == NULL) && (attributeExisted == false))  // Attribute is new but the value is missing
    {
      orionldError(OrionldBadRequestData, "Missing /languageMap/ field for LanguageProperty at creation time", attrP->name, 400);
      return false;
    }
  }
  else if (attributeType == VocabularyProperty)
  {
    if (valueP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a VocabularyProperty: value", attrP->name, 400);
      return false;
    }
    else if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a VocabularyProperty: object", attrP->name, 400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a VocabularyProperty: languageMap", attrP->name, 400);
      return false;
    }
    else if (jsonP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a VocabularyProperty: json", attrP->name, 400);
      return false;
    }
    else if ((vocabP == NULL) && (attributeExisted == false))  // Attribute is new but the value is missing
    {
      orionldError(OrionldBadRequestData, "Missing /vocab/ field for VocabularyProperty at creation time", attrP->name, 400);
      return false;
    }
  }
  else if (attributeType == JsonProperty)
  {
    if (valueP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a JsonProperty: value", attrP->name, 400);
      return false;
    }
    else if (objectP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a JsonProperty: object", attrP->name, 400);
      return false;
    }
    else if (languageMapP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a JsonProperty: languageMap", attrP->name, 400);
      return false;
    }
    else if (vocabP != NULL)
    {
      orionldError(OrionldBadRequestData, "Forbidden field for a JsonProperty: vocab", attrP->name, 400);
      return false;
    }
    else if ((jsonP == NULL) && (attributeExisted == false))  // Attribute is new but the value is missing
    {
      orionldError(OrionldBadRequestData, "Missing /json/ field for JsonProperty at creation time", attrP->name, 400);
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckLanguageMap -
//
// A languageMap must be an object with all strings inside
//
bool pCheckLanguageMap(KjNode* languageMapP, const char* attrName)
{
  PCHECK_OBJECT(languageMapP, 0, "The languageMap of a LanguageProperty attribute must be a JSON Object", attrName, 400);

  for (KjNode* langItemP = languageMapP->value.firstChildP; langItemP != NULL; langItemP = langItemP->next)
  {
    // Check the key-value pair for empty KEY
    if (langItemP->name[0] == 0)
    {
      orionldError(OrionldBadRequestData, "A key of the languageMap of a LanguageProperty is an empty string", attrName, 400);
      return false;
    }

    if (langItemP->type == KjString)
    {
      PCHECK_STRING_EMPTY(langItemP, 0, "Items of a value array of a LanguageProperty attribute cannot be an empty JSON String", attrName, 400);
    }
    else if (langItemP->type == KjArray)
    {
      for (KjNode* langValueP = langItemP->value.firstChildP; langValueP != NULL; langValueP = langValueP->next)
      {
        PCHECK_STRING(langValueP, 0, "Items of a value array of a LanguageProperty attribute must be JSON String", attrName, 400);
        PCHECK_STRING_EMPTY(langValueP, 0, "Items of a value array of a LanguageProperty attribute cannot be an empty JSON String", attrName, 400);
      }
    }
    else
    {
      PCHECK_STRING_OR_ARRAY(langItemP, 0, "Items of the value of a LanguageProperty attribute must be JSON String or Array of String", attrName, 400);
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// datasetIdCheck -
//
bool datasetIdCheck(KjNode* datasetIdP)
{
  if (datasetIdP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Invalid JSON type - not a string", datasetIdP->name, 400);
    return false;
  }

  if (pCheckUri(datasetIdP->value.s, datasetIdP->name, true) == false)
    return false;

  return true;
}



// -----------------------------------------------------------------------------
//
// objectCheck -
//
static bool objectCheck(KjNode* objectP)
{
  if (objectP->type == KjString)
  {
    if (pCheckUri(objectP->value.s, objectP->name, true) == false)
      return false;
  }
  else if (objectP->type == KjArray)
  {
    for (KjNode* uriP = objectP->value.firstChildP; uriP != NULL; uriP = uriP->next)
    {
      if (uriP->type != KjString)
      {
        orionldError(OrionldBadRequestData, "Invalid Relationship object array item - not a String", objectP->name, 400);
        return false;
      }

      if (pCheckUri(uriP->value.s, objectP->name, true) == false)
      {
        orionldError(OrionldBadRequestData, "Invalid Relationship object array item - not a URI", uriP->value.s, 400);
        return false;
      }
    }
  }
  else
  {
    orionldError(OrionldBadRequestData, "Invalid Relationship object - not a string nor an array", objectP->name, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckVocabulary -
//
static bool pCheckVocabulary(KjNode* vocabP, const char* attrName)
{
  if (vocabP->type == KjString)
  {
    vocabP->value.s = orionldContextItemExpand(orionldState.contextP, vocabP->value.s, true, NULL);
    return true;
  }

  if (vocabP->type == KjArray)
  {
    for (KjNode* wordP = vocabP->value.firstChildP; wordP != NULL; wordP = wordP->next)
    {
      if (wordP->type != KjString)
      {
        orionldError(OrionldBadRequestData, "Invalid VocabularyProperty vocab array item - not a string", attrName, 400);
        return false;
      }

      wordP->value.s = orionldContextItemExpand(orionldState.contextP, wordP->value.s, true, NULL);
    }
  }
  else
  {
    orionldError(OrionldBadRequestData, "Invalid VocabularyProperty vocab - not a string nor an array", attrName, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// unitCodeCheck -
//
bool unitCodeCheck(KjNode* unitCodeP)
{
  if (unitCodeP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Invalid JSON type - not a string", unitCodeP->name, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// timestampCheck -
//
bool timestampCheck(KjNode* fieldP)
{
  if (fieldP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Invalid JSON type - not a string (so, not a valid timestamp)", fieldP->name, 400);
    return false;
  }

  char errorString[256];
  if (dateTimeFromString(fieldP->value.s, errorString, sizeof(errorString)) < 0)
  {
    orionldError(OrionldBadRequestData, "Invalid ISO8601 timestamp", fieldP->name, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// isGeoJsonValue -
//
static bool isGeoJsonValue(KjNode* valueP)
{
  if (valueP->type != KjObject)
    return false;

  bool coordinatesPresent  = false;
  bool typePresent         = false;
  int  nodes               = 0;

  for (KjNode* nodeP = valueP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    ++nodes;

    if (nodes >= 3)
      return false;

    if (strcmp(nodeP->name, "type") == 0)
    {
      if (orionldGeometryFromString(nodeP->value.s) != GeoNoGeometry)
        typePresent = true;
    }
    else if (strcmp(nodeP->name, "coordinates") == 0)
    {
      if (nodeP->type == KjArray)
        coordinatesPresent = true;
    }
  }

  if ((typePresent == true) && (coordinatesPresent == true))
    return true;

  return false;
}


// -----------------------------------------------------------------------------
//
// multiAttributeArray -
//
bool multiAttributeArray(KjNode* attrArrayP, bool* errorP)
{
  int datasets = 0;
  int objects  = 0;

  for (KjNode* aInstanceP = attrArrayP->value.firstChildP; aInstanceP != NULL; aInstanceP = aInstanceP->next)
  {
    if (aInstanceP->type != KjObject)
      return false;

    KjNode* datasetIdP = kjLookup(aInstanceP, "datasetId");

    if (datasetIdP != NULL)
      ++datasets;

    ++objects;
  }

  if (datasets == 0)
    return false;

  if (objects - datasets > 1)  // More than one object without datasetId field
  {
      orionldError(OrionldBadRequestData, "More than one default attribute in dataset array", attrArrayP->name, 400);
      *errorP = true;
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// deletionWithTypePresent -
//
bool deletionWithTypePresent(KjNode* attrP, KjNode* typeP)
{
  KjNode* valueP;

  if ((strcmp(typeP->value.s, "Property") == 0) || (strcmp(typeP->value.s, "GeoProperty") == 0))
  {
    valueP = kjLookup(attrP, "value");
    if ((valueP != NULL) && (valueP->type == KjString) && (strcmp(valueP->value.s, "urn:ngsi-ld:null") == 0))
    {
      attrP->type = KjNull;
      return true;
    }
  }
  else if (strcmp(typeP->value.s, "Relationship") == 0)
  {
    valueP = kjLookup(attrP, "object");
    if ((valueP != NULL) && (valueP->type == KjString) && (strcmp(valueP->value.s, "urn:ngsi-ld:null") == 0))
    {
      attrP->type = KjNull;
      return true;
    }
  }
  else if (strcmp(typeP->value.s, "VocabularyProperty") == 0)
  {
    valueP = kjLookup(attrP, "vocab");
    if ((valueP != NULL) && (valueP->type == KjString) && (strcmp(valueP->value.s, "urn:ngsi-ld:null") == 0))
    {
      attrP->type = KjNull;
      return true;
    }
  }
  else if (strcmp(typeP->value.s, "LanguageProperty") == 0)
  {
    valueP = kjLookup(attrP, "languageMap");
    if ((valueP != NULL) && (valueP->type == KjObject))
    {
      KjNode* noneP = kjLookup(valueP, "@none");
      if ((noneP != NULL ) && (noneP->type == KjString) && (strcmp(noneP->value.s, "urn:ngsi-ld:null") == 0))
      {
        attrP->type = KjNull;
        return true;
      }
    }
  }
  else if (strcmp(typeP->value.s, "JsonProperty") == 0)
  {
    valueP = kjLookup(attrP, "json");
    if ((valueP != NULL) && (valueP->type == KjString) && (strcmp(valueP->value.s, "urn:ngsi-ld:null") == 0))
    {
      attrP->type = KjNull;
      return true;
    }
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// deletionWithoutTypePresent -
//
static bool deletionWithoutTypePresent
(
  KjNode*               attrP,
  OrionldAttributeType  attributeType,
  KjNode*               valueP,
  KjNode*               objectP,
  KjNode*               languageMapP,
  KjNode*               vocabP,
  KjNode*               jsonP
)
{
  if ((attributeType == Property) || (attributeType == GeoProperty))
  {
    if ((valueP != NULL) && (valueP->type == KjString) && (strcmp(valueP->value.s, "urn:ngsi-ld:null") == 0))
    {
      attrP->type = KjNull;
      return true;
    }
  }
  else if (attributeType == Relationship)
  {
    if ((objectP != NULL) && (objectP->type == KjString) && (strcmp(objectP->value.s, "urn:ngsi-ld:null") == 0))
    {
      attrP->type = KjNull;
      return true;
    }
  }
  else if (attributeType == VocabularyProperty)
  {
    if ((vocabP != NULL) && (vocabP->type == KjString) && (strcmp(vocabP->value.s, "urn:ngsi-ld:null") == 0))
    {
      attrP->type = KjNull;
      return true;
    }
  }
  else if (attributeType == LanguageProperty)
  {
    if ((languageMapP != NULL) && (languageMapP->type == KjObject))
    {
      KjNode* noneP = kjLookup(languageMapP, "@none");
      if ((noneP != NULL ) && (noneP->type == KjString) && (strcmp(noneP->value.s, "urn:ngsi-ld:null") == 0))
      {
        attrP->type = KjNull;
        return true;
      }
    }
  }
  else if (attributeType == JsonProperty)
  {
    if ((jsonP != NULL) && (jsonP->type == KjString) && (strcmp(jsonP->value.s, "urn:ngsi-ld:null") == 0))
    {
      attrP->type = KjNull;
      return true;
    }
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// isJsonLiteral -
//
// FIXME: move to its own module
//
static bool isJsonLiteral(KjNode* attrP, KjNode* typeP)
{
  if (typeP == NULL)
  {
    typeP = kjLookup(attrP, "@type");
    if (typeP == NULL)
      return false;
  }
  else if (strcmp(typeP->name, "@type") != 0)
    return false;

  if (strcmp(typeP->value.s, "@json") != 0)
    return false;

  // Must have an @value also
  if (kjLookup(attrP, "@value") == NULL)
    return false;

  return true;
}



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
static bool pCheckAttributeObject
(
  const char*           entityId,                // For log messages only, still important
  KjNode*               attrP,
  bool                  isAttribute,
  OrionldAttributeType  attrTypeFromDb,
  OrionldContextItem*   attrContextInfoP
)
{
  OrionldAttributeType  attributeType = NoAttributeType;
  KjNode*               typeP;

  // Check for errors in the input payload for the attribute type
  if (pCheckAttributeType(attrP, &typeP, false) == false)
    return false;

  //
  // If the attribute already exists (in the database), we KNOW the type of the attribute.
  // If the payload body contains a type, and it's not the same type, well, that's an error right there
  //
  // However, the payload body *might* be the value of a GeoProperty.
  // If so, the payload body would have a type == "Point", "Polygon" or something similar.
  // We CAN'T compare the type with what "should be according to the DB" until we rule GeoProperty value out.
  //
  // That is done a little later in this function - see "All good, let's now compare with the truth"
  //
  if (typeP != NULL)  // Attribute Type given in incoming payload
  {
    //
    // Check for Deletion -
    // FIXME: Change ORIONLD_SERVICE_OPTION_ACCEPT_JSONLD_NULL for ... ORIONLD_SERVICE_OPTION_xxxxx
    //
    if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_ACCEPT_JSONLD_NULL) != 0)
    {
      if (deletionWithTypePresent(attrP, typeP) == true)
        return true;
    }

    //
    // If type is in the payload:
    //   Is it valid?
    //   - Either "Property", "Relationhip", etc
    //   - OR, GeoJSON type (Point, Polygon) AND coordinates member and only those two   => GeoProperty
    //   - IF NOT valid, then "Attribute Type Error"
    //   - IF VALID, if also 'attrTypeFromDb != NoAttributeType', compare => "attempt to change attr type"
    //
    bool geoJsonValue = false;

    attributeType = orionldAttributeType(typeP->value.s);
    if (attributeType == NoAttributeType)
    {
      if (isGeoJsonValue(attrP))
      {
        attributeType = GeoProperty;
        geoJsonValue  = true;
      }
      else if (isJsonLiteral(attrP, typeP) == true)
      {
        KjNode* typeP  = kjString(orionldState.kjsonP, "type", "Property");
        KjNode* valueP = kjObject(orionldState.kjsonP, "value");

        // Steal children from attrP and put them in valueP
        valueP->value.firstChildP = attrP->value.firstChildP;
        valueP->lastChild         = attrP->lastChild;

        // valueP stole the entire RHS - now empty the RHS of attrP
        attrP->value.firstChildP = NULL;
        attrP->lastChild         = NULL;

        // Finally, add typeP and valueP to attrP
        kjChildAdd(attrP, typeP);
        kjChildAdd(attrP, valueP);

        return true;
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid value for attribute /type/", typeP->value.s, 400);
        return false;
      }
    }

    if ((attrTypeFromDb != NoAttributeType) && (attributeType != attrTypeFromDb))
    {
      // This might be OK, if keyValues is ON => attributeType is a guess
      if (orionldState.out.format != RF_SIMPLIFIED)
      {
        const char* title = attrTypeChangeTitle(attrTypeFromDb, attributeType);
        orionldError(OrionldBadRequestData, title, attrP->name, 400);
        return false;
      }
    }

    if (geoJsonValue == true)
    {
      // It's a GeoProperty in key-values format - need to convert to Normalized
      KjNode* valueP = kjObject(orionldState.kjsonP,  "value");

      valueP->value.firstChildP = attrP->value.firstChildP;
      valueP->lastChild         = attrP->lastChild;

      pCheckAttributeTransform(attrP, "GeoProperty", valueP);
    }
    else
    {
      // As "type" is present - is it coherent? (Property has "value", Relationship has "object", etc)
      if (valueAndTypeCheck(attrP, attributeType, attrTypeFromDb != NoAttributeType) == false)
        LM_RE(false, ("valueAndTypeCheck failed"));
    }
  }
  else  // Attribute Type is not there - try to guess the type, if not already known from the DB
  {
    KjNode* valueP       = kjLookup(attrP, "value");
    KjNode* objectP      = kjLookup(attrP, "object");
    KjNode* languageMapP = kjLookup(attrP, "languageMap");
    KjNode* vocabP       = kjLookup(attrP, "vocab");
    KjNode* jsonP        = kjLookup(attrP, "json");

    if (valueP != NULL)
    {
      if (isGeoJsonValue(valueP) == true)
        attributeType = GeoProperty;
      else
      {
        attributeType = Property;
        arrayReduce(valueP);
      }
    }
    else if (objectP != NULL)
    {
      attributeType = Relationship;
      arrayReduce(objectP);
    }
    else if (languageMapP != NULL)
      attributeType = LanguageProperty;
    else if (vocabP != NULL)
    {
      attributeType = VocabularyProperty;
      arrayReduce(vocabP);
    }
    else if (jsonP != NULL)
    {
      attributeType = JsonProperty;
      arrayReduce(jsonP);
    }
    else
    {
      // If new attribute and no value field at all - error
      if (attrTypeFromDb == NoAttributeType)
      {
        orionldError(OrionldBadRequestData, "Missing value/object/languageMap field for Attribute at creation time", attrP->name, 400);
        return false;
      }
    }

    if (valueAndTypeCheck(attrP, attributeType, attrTypeFromDb != NoAttributeType) == false)
      LM_RE(false, ("valueAndTypeCheck failed"));

    if ((attrTypeFromDb != NoAttributeType) && (attributeType != NoAttributeType) && (attributeType != attrTypeFromDb))
    {
      // This might be OK, if keyValues is ON => attributeType is a guess
      if (orionldState.out.format != RF_SIMPLIFIED)
      {
        const char* title = attrTypeChangeTitle(attrTypeFromDb, attributeType);
        orionldError(OrionldBadRequestData, title, attrP->name, 400);
        return false;
      }
    }

    //
    // Check for Deletion -
    // FIXME: Change ORIONLD_SERVICE_OPTION_ACCEPT_JSONLD_NULL for ... ORIONLD_SERVICE_OPTION_xxxxx
    //
    if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_ACCEPT_JSONLD_NULL) != 0)
    {
      if (deletionWithoutTypePresent(attrP, attributeType, valueP, objectP, languageMapP, vocabP, jsonP) == true)
        return true;
    }
  }

  KjNode* fieldP = attrP->value.firstChildP;
  KjNode* next;

  while (fieldP != NULL)
  {
    next = fieldP->next;

    if ((fieldP->type == KjString) && (strcmp(fieldP->value.s, "urn:ngsi-ld:null") == 0))
      fieldP->type = KjNull;

    if (fieldP->type == KjNull)
    {
      if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_ACCEPT_JSONLD_NULL) == 0)
      {
        char errorString[512];
        snprintf(errorString, sizeof(errorString),
                 "Got a JSON 'null' in RHS (not allowed in JSON-LD docs) for the entity '%s', attribute '%s', attribute field '%s'",
                 entityId, attrP->name, fieldP->name);
        LM_E(("%s", errorString));
        orionldError(OrionldBadRequestData, "Bad Input", errorString, 400);
        return false;
      }

      fieldP = next;
      continue;  // NULL, all good. Next!
    }

    if (fieldP == typeP)
    {
      // The value/object/languageMap is left as is
    }
    else if (strcmp(fieldP->name, "value") == 0)
    {
      if (fieldP->type == KjArray)
      {
        if ((fieldP->value.firstChildP != NULL) && (fieldP->value.firstChildP->next == NULL))
        {
          fieldP->lastChild = fieldP->value.firstChildP->lastChild;  // Might be an array or object inside the array ...
          fieldP->type      = fieldP->value.firstChildP->type;
          fieldP->value     = fieldP->value.firstChildP->value;
        }
      }
      if ((attributeType == Relationship) || (attributeType == LanguageProperty) || (attributeType == VocabularyProperty) || (attributeType == JsonProperty))
      {
        orionldError(OrionldBadRequestData, "Invalid member /value/", "valid for Property/GeoProperty attributes only", 400);
        return false;
      }
    }
    else if (strcmp(fieldP->name, "type") == 0)
    {
      // Sub-attribute type, or GeoProperty-type inside value
    }
    else if (strcmp(fieldP->name, "object") == 0)
    {
      if (attributeType == Relationship)
      {
        if (objectCheck(fieldP) == false)
          return false;

        // All OK - clear part errors
        orionldState.pd.status = 200;
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid member /object/", "valid for Relationship attributes only", 400);
        return false;
      }
    }
    else if ((attributeType == LanguageProperty) && (strcmp(fieldP->name, "languageMap") == 0))
    {
      if (pCheckLanguageMap(fieldP, attrP->name) == false)
        return false;
    }
    else if ((attributeType == VocabularyProperty) && (strcmp(fieldP->name, "vocab") == 0))
    {
      if (pCheckVocabulary(fieldP, attrP->name) == false)
        return false;
    }
    else if ((attributeType == JsonProperty) && (strcmp(fieldP->name, "json") == 0))
    {
    }
    else if (strcmp(fieldP->name, "observedAt") == 0)
    {
      //
      // FIXME: Forbid?   (400 Bad Request)
      //
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
      if (((attributeType == Property) || (attributeType == NoAttributeType)) && ((attrTypeFromDb == Property) || (attrTypeFromDb == NoAttributeType)))
      {
        if (unitCodeCheck(fieldP) == false)
          return false;
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid member /unitCode/", "valid for Property attributes only", 400);
        return false;
      }
    }
    else
    {
      if (pCheckAttribute(entityId, fieldP, false, NoAttributeType, false, NULL) == false)
        return false;
    }

    //
    // Add the Attribute Type field, if not present, and, if fully known!
    //
    if ((typeP == NULL) && (attributeType != NoAttributeType))
    {
      typeP = kjString(orionldState.kjsonP, "type", orionldAttributeTypeName(attributeType));
      kjChildAdd(attrP, typeP);
    }

    fieldP = next;
  }

  if (attributeType == GeoProperty)
  {
    if (pCheckGeoProperty(attrP) == false)
    {
      LM_W(("pCheckGeoProperty flagged an error: %s: %s", orionldState.pd.title, orionldState.pd.detail));
      return false;
    }

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

    orionldState.geoAttrV[orionldState.geoAttrs++] = attrP;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// validAttrName -
//
static bool validAttrName(const char* attrName, bool isAttribute)
{
  bool  ok = true;

  if (isAttribute == true)
  {
    if      (strcmp(attrName, "id")    == 0)  ok = false;
    else if (strcmp(attrName, "@id")   == 0)  ok = false;
    else if (strcmp(attrName, "type")  == 0)  ok = false;
    else if (strcmp(attrName, "@type") == 0)  ok = false;
    else if (strcmp(attrName, "scope") == 0)  ok = false;
  }
  else
  {
    if      (strcmp(attrName, "type")  == 0)  ok = false;
    else if (strcmp(attrName, "@type") == 0)  ok = false;
  }

  if (ok == false)
  {
    const char* title = (isAttribute == true)? "Forbidden attribute name" : "Forbidden sub-attribute name";
    orionldError(OrionldInternalError, title, attrName, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckAttribute -
//
// PARAMETERS
// attrTypeFromDb -    needed, only for PATCH Entity/Attribute, to make sure
//                     the attribute update isn't trying to modify the type of the attribute.
//                     API endpoints other than those two need not make this check as attributes are REPLACED.
//                     Likewise, in the second (recursive) call to pCheckAttribute for PATCH Attribute, it is not
//                     needed as all sub-attributes are REPLACED.
//
// -----------------------------------------------------------------------------
//
// With Simplified Format, an attribute RHS can be in a variety of formats:
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
//  - Attributes of type VocabularyProperty can have the following special attributes:
//      - type
//      - vocab
//      - observedAt
//      - datasetId (sub-attributes don't have datasetId)
//
//  - Attributes of type JsonProperty can have the following special attributes:
//      - type
//      - json
//      - observedAt
//      - datasetId (sub-attributes don't have datasetId)
//
bool pCheckAttribute
(
  const char*             entityId,
  KjNode*                 attrP,
  bool                    isAttribute,
  OrionldAttributeType    attrTypeFromDb,
  bool                    attrNameAlreadyExpanded,
  OrionldContextItem*     attrContextInfoP          // Attr Info from the @context (add shortname to struct?)
)
{
  if (attrNameAlreadyExpanded == false)
  {
    if (pCheckName(attrP->name) == false)
      return false;
    if (pCheckUri(attrP->name, attrP->name, false) == false)  // FIXME: Both pCheckName and pCheckUri check for forbidden chars ...
      return false;

    if (isAttribute)
      attrP->name = orionldAttributeExpand(orionldState.contextP, attrP->name, true, &attrContextInfoP);
    else
      attrP->name = orionldSubAttributeExpand(orionldState.contextP, attrP->name, true, &attrContextInfoP);
  }

  // "Direct" Deletion?
  if ((attrP->type == KjString) && (strcmp(attrP->value.s, "urn:ngsi-ld:null") == 0))
  {
    attrP->type = KjNull;
    return true;
  }

  if (pCheckTypeFromContext(attrP, attrContextInfoP) == false)
    return false;

  // Invalid name for attribute/sub-attribute?
  if (validAttrName(attrP->name, isAttribute) == false)
    return false;

  if ((isAttribute == true) && (attrP->type == KjArray))
  {
    bool error = false;

    if (multiAttributeArray(attrP, &error) == true)
    {
      for (KjNode* aInstanceP = attrP->value.firstChildP; aInstanceP != NULL; aInstanceP = aInstanceP->next)
      {
        // Do I need the attribute instance in the DB?
        // - only need it to determine the type of the attribute (I think ...)
        // If so, the "non-datasetId" attribute is sufficient
        // - What if there is no "non-datasetId" attr ... ?
        // - Might need the datasetsP anyway
        // - Would be good also (set to NULL in recursive calls, to avoid enter here again - datasetId check)
        // - Actually, cannot be an Array inside there, so that problem is already taken care of
        //
        // => KjNode* datasetsP should be a parameter to this function
        //
        aInstanceP->name = attrP->name;
        if (pCheckAttribute(entityId, aInstanceP, true, attrTypeFromDb, attrNameAlreadyExpanded, attrContextInfoP) == false)
          return false;
      }

      return true;
    }
    else
    {
      if (error == true)  // Early detection of erroneous datasetId-array
        return false;
    }
  }

  //
  // Check for special attributes
  //
  // Attributes
  //   If isAttribute == true, we're on Entity Level. There are 3 special attributes, all GeoProperty
  //   As we don't know the type yet, that check is postponed to pCheckAttributeObject
  //
  // Sub-Attributes
  //   If isAttribute == false, we're on Attribute level. This is even more complex.
  //   We don't know which sub-attributes are special until we know the type of the attribute (Property, Relationship, etc)
  //   Postponed to ... all simple Property functions - pCheckAttribute[String|Integer|Float|Boolean] + pCheckAttributeObject (Geo)
  //
  if      (attrP->type == KjString)  return pCheckAttributeString(entityId,  attrP, isAttribute, attrTypeFromDb, attrContextInfoP);
  else if (attrP->type == KjInt)     return pCheckAttributeInteger(entityId, attrP, isAttribute, attrTypeFromDb, attrContextInfoP);
  else if (attrP->type == KjFloat)   return pCheckAttributeFloat(entityId,   attrP, isAttribute, attrTypeFromDb, attrContextInfoP);
  else if (attrP->type == KjBoolean) return pCheckAttributeBoolean(entityId, attrP, isAttribute, attrTypeFromDb, attrContextInfoP);
  else if (attrP->type == KjArray)   return pCheckAttributeArray(entityId,   attrP, isAttribute, attrTypeFromDb, attrContextInfoP);
  else if (attrP->type == KjObject)  return pCheckAttributeObject(entityId,  attrP, isAttribute, attrTypeFromDb, attrContextInfoP);
  else if (attrP->type == KjNull)    return pCheckAttributeNull(entityId,    attrP);

  // Invalid JSON type of the attribute - we should never reach this point
  orionldError(OrionldInternalError, "invalid value type for attribute", attrP->name, 500);

  return false;
}
