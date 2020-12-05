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
#include "orionld/types/AttributeType.h"                         // AttributeType



// -----------------------------------------------------------------------------
//
// attributeTypeName -
//
const char* attributeTypeName(AttributeType aType)
{
  switch (aType)
  {
  case ATTRIBUTE_ANY:                return "ANY";
  case ATTRIBUTE_CREATED_AT:         return "CREATED_AT";
  case ATTRIBUTE_MODIFIED_AT:        return "MODIFIED_AT";
  case ATTRIBUTE_LOCATION:           return "LOCATION";
  case ATTRIBUTE_OBSERVATION_SPACE:  return "OBSERVATION_SPACE";
  case ATTRIBUTE_OPERATION_SPACE:    return "OPERATION_SPACE";
  case ATTRIBUTE_OBSERVED_AT:        return "OBSERVED_AT";
  case ATTRIBUTE_DATASETID:          return "DATASETID";
  case ATTRIBUTE_INSTANCEID:         return "INSTANCEID";
  case ATTRIBUTE_UNITCODE:           return "UNITCODE";
  case ATTRIBUTE_PROPERTY:           return "PROPERTY";
  case ATTRIBUTE_GEO_PROPERTY:       return "GEO_PROPERTY";
  case ATTRIBUTE_RELATIONSHIP:       return "RELATIONSHIP";
  }

  return "unknown attribute type";
}
