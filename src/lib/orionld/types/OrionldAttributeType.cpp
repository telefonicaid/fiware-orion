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

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldAttributeType.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// orionldAttributeTypeName -
//
const char* orionldAttributeTypeName(OrionldAttributeType attributeType)
{
  switch (attributeType)
  {
  case NoAttributeType:     return "NoAttributeType";
  case Property:            return "Property";
  case Relationship:        return "Relationship";
  case GeoProperty:         return "GeoProperty";
  case LanguageProperty:    return "LanguageProperty";
  case VocabularyProperty:  return "VocabularyProperty";
  }

  return "InvalidAttributeType";
}



// -----------------------------------------------------------------------------
//
// orionldAttributeType -
//
OrionldAttributeType orionldAttributeType(const char* typeString)
{
  if      (strcmp(typeString, "Property")            == 0) return Property;
  else if (strcmp(typeString, "Relationship")        == 0) return Relationship;
  else if (strcmp(typeString, "GeoProperty")         == 0) return GeoProperty;
  else if (strcmp(typeString, "LanguageProperty")    == 0) return LanguageProperty;
  else if (strcmp(typeString, "VocabularyProperty")  == 0) return VocabularyProperty;

  return NoAttributeType;
}
