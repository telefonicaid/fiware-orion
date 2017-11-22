#ifndef SRC_LIB_ORIONTYPES_ORIONVALUETYPE_H_
#define SRC_LIB_ORIONTYPES_ORIONVALUETYPE_H_

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



namespace orion
{

/* ****************************************************************************
*
* ValueType - 
*
* Note that ValueTypeNotGiven is only used by NGSIv1 parsing logic. In NGSIv2 parsing,
* attribute or metatadata value is implicit if omitted (in particular, it
* takes null value in that case). However, this could change if we implement at
* some point PATCH on attribute and metadata. In that case, we would need to
* use ValueTypeNotGiven also in the cases value is omitted.
*
*/
typedef enum ValueType
{
  ValueTypeString,
  ValueTypeNumber,
  ValueTypeBoolean,
  ValueTypeVector,
  ValueTypeObject,
  ValueTypeNull,
  ValueTypeNotGiven
} ValueType;



/* ****************************************************************************
*
* valueTypeName - 
*/
extern const char* valueTypeName(const orion::ValueType _type);


/* ****************************************************************************
*
* defaultType - 
*/
extern const char* defaultType(orion::ValueType valueType);

}
#endif  // SRC_LIB_ORIONTYPES_ORIONVALUETYPE_H_
