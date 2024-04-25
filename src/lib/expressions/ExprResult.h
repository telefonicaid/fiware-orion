#ifndef SRC_LIB_EXPRESSIONS_EXPRRESULT_H_
#define SRC_LIB_EXPRESSIONS_EXPRRESULT_H_

/*
*
* Copyright 2024 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*/

#include "orionTypes/OrionValueType.h"

#include "parse/CompoundValueNode.h"

#include <string>

/* ****************************************************************************
*
* ExprResult -
*/
class ExprResult
{
public:
  // Similar to the fields used in ContextAttribute.h
  
  orion::ValueType  valueType;    // Type of value
  std::string       stringValue;  // "value" as a String
  double            numberValue;  // "value" as a Number
  bool              boolValue;    // "value" as a Boolean

  // Use only when valueType is object or vector
  orion::CompoundValueNode*  compoundValueP;

  ExprResult(void);

  void fill(const std::string& result);

  std::string  toString(void);
  void         release(void);
};



#endif  // SRC_LIB_EXPRESSIONS_EXPRRESULT_H_
