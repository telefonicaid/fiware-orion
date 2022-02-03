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


static bool pcheckLanguagePropertyValue(KjNode* attrP, OrionldProblemDetails* pdP) { return true; }
static bool pcheckGeoPropertyValue(KjNode* attrP, OrionldProblemDetails* pdP) { return true; }
static bool pcheckRelationshipObject(KjNode* attrP, OrionldProblemDetails* pdP) { return true; }
static bool pcheckPropertyValue(KjNode* attrP, OrionldProblemDetails* pdP) { return true; }



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



// -----------------------------------------------------------------------------
//
// pbodyAttribute -
//
bool pbodyAttribute(KjNode* attrP, OrionldProblemDetails* pdP)
{
  LM_TMP(("KZ: attrP is of JSON type %s", kjValueType(attrP->type)));

  if (attrP->type == KjObject)
  {
    //
    // Might be the "value" of a GeoProperty ( type + coordinates must be present )
    // Might be the "value" of a LanguageProperty  ( all fields are strings ... a bit risky ...)
    // Normally it's the RHS of an attribute - { "type": XX, "value": XX, ... }
    // If RHS, we must loop over all fields and check + fill in missing fields
    //

    return pbodyAttributeAsObject(attrP, pdP);
  }
  else if (attrP->type == KjArray)
  {
    // Check for datasetId !!!

    KjNode* valueP = kjArray(orionldState.kjsonP, "value");
    KjNode* typeP  = kjString(orionldState.kjsonP,  "type",  "Property");

    valueP->value.firstChildP = attrP->value.firstChildP;
    valueP->lastChild         = attrP->lastChild;

    kjObjectTransform(attrP, NULL, NULL);
    kjChildAdd(attrP, typeP);
    kjChildAdd(attrP, valueP);
  }
  else if (attrP->type == KjInt)
  {
    KjNode* valueP = kjInteger(orionldState.kjsonP, "value", attrP->value.i);
    KjNode* typeP  = kjString(orionldState.kjsonP,  "type",  "Property");

    kjObjectTransform(attrP, NULL, NULL);
    kjChildAdd(attrP, typeP);
    kjChildAdd(attrP, valueP);
  }
  else if (attrP->type == KjString)
  {
    bool    relationship = (strncmp(attrP->value.s, "urn:ngsi-ld:", 12) == 0);
    char*   valueKey     = (relationship == false)? (char*) "value" :  (char*) "object";
    char*   attrType     = (relationship == false)? (char*) "Property" : (char*) "Relationship";
    KjNode* valueP       = kjString(orionldState.kjsonP, valueKey, attrP->value.s);
    KjNode* typeP        = kjString(orionldState.kjsonP, "type",  attrType);

    kjObjectTransform(attrP, NULL, NULL);
    kjChildAdd(attrP, typeP);
    kjChildAdd(attrP, valueP);
  }
  else if (attrP->type == KjFloat)
  {
    KjNode* valueP = kjFloat(orionldState.kjsonP,  "value", attrP->value.f);
    KjNode* typeP  = kjString(orionldState.kjsonP, "type",  "Property");

    kjObjectTransform(attrP, NULL, NULL);
    kjChildAdd(attrP, typeP);
    kjChildAdd(attrP, valueP);
  }
  else if (attrP->type == KjBoolean)
  {
    KjNode* valueP = kjBoolean(orionldState.kjsonP,  "value", attrP->value.b);
    KjNode* typeP  = kjString(orionldState.kjsonP, "type",  "Property");

    kjObjectTransform(attrP, NULL, NULL);
    kjChildAdd(attrP, typeP);
    kjChildAdd(attrP, valueP);
  }

  return true;
}
