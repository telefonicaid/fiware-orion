/*
*
* Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/

#include "mongoDriver/BSONTypes.h"

#include "logMsg/logMsg.h"

/* ****************************************************************************
*
* BSONObjBuilder::BSONObjBuilder -
*/
const char* orion::bsonType2String(orion::BSONType t)
{
  switch (t)
  {
  case EOO:           return "EOO";
  case NumberDouble:  return "NumberDouble";
  case String:        return "String";
  case Object:        return "Object";
  case Array:         return "Array";
  case BinData:       return "BinData";
  case Undefined:     return "Undefined";
  case jstOID:        return "jstOID";
  case Bool:          return "Bool";
  case Date:          return "Date";
  case jstNULL:       return "jstNULL";
  case RegEx:         return "RegEx";
  case DBRef:         return "DBRef";
  case Code:          return "Code";
  case Symbol:        return "Symbol";
  case CodeWScope:    return "CodeWScope";
  case NumberInt:     return "NumberInt";
  case Timestamp:     return "Timestamp";
  case NumberLong:    return "NumberLong";
  case BigDecimal:    return "BigDecimal";
  case MinKey:        return "MinKey";
  case MaxKey:        return "MaxKey";
  }

  LM_E(("Runtime Error (unknown BSONType: %d", t));
  return "Unknown";
}
