/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "common/globals.h"
#include "orionTypes/OrionValueType.h"


namespace orion
{



/* ****************************************************************************
*
* valueTypeName -
*/
const char* valueTypeName(const orion::ValueType _type)
{
  switch (_type)
  {
  case ValueTypeString:       return "String";
  case ValueTypeNumber:       return "Number";
  case ValueTypeBoolean:      return "Boolean";
  case ValueTypeObject:       return "Object";
  case ValueTypeVector:       return "Vector";
  case ValueTypeNull:         return "Null";
  case ValueTypeNotGiven:     return "NotGiven";
  }

  return "Invalid";
}



/* ****************************************************************************
*
* defaultType -
*/
const char* defaultType(ValueType valueType)
{
  switch (valueType)
  {
  case ValueTypeString:     return DEFAULT_ATTR_STRING_TYPE;
  case ValueTypeNumber:     return DEFAULT_ATTR_NUMBER_TYPE;
  case ValueTypeBoolean:    return DEFAULT_ATTR_BOOL_TYPE;
  case ValueTypeVector:     return DEFAULT_ATTR_ARRAY_TYPE;
  case ValueTypeObject:     return DEFAULT_ATTR_OBJECT_TYPE;
  case ValueTypeNull:       return DEFAULT_ATTR_NULL_TYPE;
  case ValueTypeNotGiven:   return "NotGiven";
  }

  return "Unknown";
}

}
