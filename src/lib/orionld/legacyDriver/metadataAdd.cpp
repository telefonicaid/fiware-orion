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
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "ngsi/ContextAttribute.h"                               // ContextAttribute

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/legacyDriver/kjTreeToCompoundValue.h"          // kjTreeToCompoundValue
#include "orionld/legacyDriver/metadataAdd.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// metadataValueSet - set the value of a Metadata instance
//
static bool metadataValueSet(Metadata* mdP, KjNode* valueNodeP)
{
  switch (valueNodeP->type)
  {
  case KjBoolean:    mdP->valueType = orion::ValueTypeBoolean; mdP->boolValue      = valueNodeP->value.b; break;
  case KjInt:        mdP->valueType = orion::ValueTypeNumber;  mdP->numberValue    = valueNodeP->value.i; break;
  case KjFloat:      mdP->valueType = orion::ValueTypeNumber;  mdP->numberValue    = valueNodeP->value.f; break;
  case KjString:     mdP->valueType = orion::ValueTypeString;  mdP->stringValue    = valueNodeP->value.s; break;
  case KjObject:     mdP->valueType = orion::ValueTypeObject;  mdP->compoundValueP = kjTreeToCompoundValue(valueNodeP, NULL, 0); break;
  case KjArray:      mdP->valueType = orion::ValueTypeObject;  mdP->compoundValueP = kjTreeToCompoundValue(valueNodeP, NULL, 0);  break;
  case KjNull:       mdP->valueType = orion::ValueTypeNull;    break;
  case KjNone:
    LM_E(("Invalid json type (KjNone!) for value field of metadata '%s'", valueNodeP->name));
    orionldError(OrionldBadRequestData, "Bad Request", "Invalid JSON type", 400);
    return false;
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// metadataAdd -
//
bool metadataAdd(ContextAttribute* caP, KjNode* nodeP, char* attributeName)
{
  KjNode*   typeNodeP       = NULL;
  KjNode*   valueNodeP      = NULL;
  KjNode*   objectNodeP     = NULL;
  KjNode*   observedAtP     = NULL;
  KjNode*   unitCodeP       = NULL;
  bool      isProperty      = false;
  bool      isRelationship  = false;
  char*     shortName       = orionldContextItemAliasLookup(orionldState.contextP, nodeP->name, NULL, NULL);

  if (SCOMPARE11(shortName, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
  {
    isProperty  = true;
    observedAtP = nodeP;
    valueNodeP  = nodeP;
  }
  else if (SCOMPARE9(shortName, 'u', 'n', 'i', 't', 'C', 'o', 'd', 'e', 0))
  {
    isProperty  = true;
    unitCodeP   = nodeP;
    valueNodeP  = nodeP;
  }
  else if (nodeP->type == KjObject)
  {
    for (KjNode* kNodeP = nodeP->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
    {
      if (strcmp(kNodeP->name, "type") == 0)
      {
        DUPLICATE_CHECK(typeNodeP, "metadata type", kNodeP);
        STRING_CHECK(kNodeP, "metadata type");

        if (strcmp(kNodeP->value.s, "Property") == 0)
          isProperty = true;
        else if (strcmp(kNodeP->value.s, "GeoProperty") == 0)
          isProperty = true;
        else if (strcmp(kNodeP->value.s, "Relationship") == 0)
          isRelationship = true;
        else if (strcmp(kNodeP->value.s, "LanguageProperty") == 0)
          isProperty = true;
        else
        {
          LM_E(("Invalid type for metadata '%s': '%s'", kNodeP->name, kNodeP->value.s));
          orionldError(OrionldBadRequestData, "Invalid type for sub-attribute", kNodeP->value.s, 400);
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

    if ((typeNodeP == NULL) && (observedAtP == NULL))  // "observedAt" is a special metadata that has no type, just a value.
    {
      orionldError(OrionldBadRequestData, "The 'type' field is missing for a sub-attribute", nodeP->name, 400);
      return false;
    }

    if ((isProperty == true) && (valueNodeP == NULL))
    {
      orionldError(OrionldBadRequestData, "The value field is missing for a Property metadata", nodeP->name, 400);
      return false;
    }
    else if ((isRelationship == true) && (objectNodeP == NULL))
    {
      orionldError(OrionldBadRequestData, "The 'object' field is missing for a Relationship metadata", nodeP->name, 400);
      return false;
    }
  }
  else
  {
    isProperty = true;
    valueNodeP = nodeP;
  }

  Metadata* mdP = NULL;
  try
  {
    mdP = new Metadata();
    mdP->name = nodeP->name;
  }
  catch (...)
  {
    LM_E(("caught exception from 'new Metadata' - out of memory creating property/relationship for an attribute"));
    mdP = NULL;
  }

  if (mdP == NULL)
  {
    orionldError(OrionldInternalError, "Out of memory creating property/relationship for attribute", nodeP->name, 500);
    return false;
  }

  if (typeNodeP != NULL)  // Only if the metadata is a JSON Object
    mdP->type = typeNodeP->value.s;

  if (observedAtP != NULL)
  {
    mdP->valueType   = orion::ValueTypeNumber;

    if (observedAtP->type == KjObject)
    {
      // Lookup member "value"
      KjNode* valueP = kjLookup(observedAtP, "value");
      if (valueP != NULL)
      {
        if (valueP->type == KjFloat)
          mdP->numberValue = valueP->value.f;
        else
          mdP->numberValue = valueP->value.i;
      }
      else
        mdP->numberValue = 0;
    }
    else
    {
      if (observedAtP->type == KjFloat)
        mdP->numberValue = observedAtP->value.f;
      else
        mdP->numberValue = observedAtP->value.i;
    }
  }
  else if (unitCodeP != NULL)
  {
    mdP->valueType   = orion::ValueTypeString;
    mdP->name        = "unitCode";

    if (unitCodeP->type == KjObject)
    {
      // Lookup member "value"
      KjNode* valueP = kjLookup(unitCodeP, "value");
      if (valueP != NULL)
        mdP->stringValue = valueP->value.s;
      else
      {
        LM_E(("Internal Error (no value field from DB)"));
        mdP->stringValue = "value lost in DB";
      }
    }
    else
      mdP->stringValue = unitCodeP->value.s;
  }
  else if (isProperty == true)
  {
    if (metadataValueSet(mdP, valueNodeP) == false)
    {
      // metadataValueSet calls orionldError
      delete mdP;
      return false;
    }
  }
  else  // Relationship
  {
    // A "Relationship" has no value, instead it has 'object', that must be of string type
    if (objectNodeP->type != KjString)
    {
      orionldError(OrionldInternalError, "invalid json type for relationship-object", nodeP->name, 400);
      delete mdP;
      return false;
    }

    mdP->valueType   = orion::ValueTypeString;
    mdP->stringValue = objectNodeP->value.s;
  }

  caP->metadataVector.push_back(mdP);

  return true;
}
