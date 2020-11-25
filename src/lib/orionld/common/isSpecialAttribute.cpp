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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/types/AttributeType.h"                         // AttributeType
#include "orionld/common/isSpecialAttribute.h"                   // Own interface



// ----------------------------------------------------------------------------
//
// isSpecialAttribute -
//
bool isSpecialAttribute(const char* attrName, AttributeType* aTypeP, KjNode* attributeTypeNodeP)
{
  *aTypeP = ATTRIBUTE_ANY;  // This indicates an error

  if (attributeTypeNodeP != NULL)
  {
    if (strcmp(attributeTypeNodeP->value.s, "Property") == 0)
      *aTypeP = ATTRIBUTE_PROPERTY;
    else if (strcmp(attributeTypeNodeP->value.s, "GeoProperty") == 0)
      *aTypeP = ATTRIBUTE_GEO_PROPERTY;
    else if (strcmp(attributeTypeNodeP->value.s, "Relationship") == 0)
      *aTypeP = ATTRIBUTE_RELATIONSHIP;
  }

  if (strcmp(attrName, "createdAt") == 0)
  {
    *aTypeP = ATTRIBUTE_CREATED_AT;
    return true;
  }
  else if (strcmp(attrName, "modifiedAt") == 0)
  {
    *aTypeP = ATTRIBUTE_MODIFIED_AT;
    return true;
  }
  if (strcmp(attrName, "location") == 0)
  {
    *aTypeP = ATTRIBUTE_LOCATION;
    return true;
  }

  if (strcmp(attrName, "observationSpace") == 0)
  {
    *aTypeP = ATTRIBUTE_OBSERVATION_SPACE;
    return true;
  }

  if (strcmp(attrName, "operationSpace") == 0)
  {
    *aTypeP = ATTRIBUTE_OPERATION_SPACE;
    return true;
  }

  return false;
}
