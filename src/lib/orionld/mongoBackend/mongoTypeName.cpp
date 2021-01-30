/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include "mongo/client/dbclient.h"                        // mongo::BSONType



// -----------------------------------------------------------------------------
//
// mongoTypeName -
//
const char* mongoTypeName(mongo::BSONType type)
{
  switch (type)
  {
  case mongo::MinKey:        return "MinKey";
  case mongo::EOO:           return "EOO";
  case mongo::NumberDouble:  return "NumberDouble";
  case mongo::String:        return "String";
  case mongo::Object:        return "Object";
  case mongo::Array:         return "Array";
  case mongo::BinData:       return "BinData";
  case mongo::Undefined:     return "Undefined";
  case mongo::jstOID:        return "jstOID";
  case mongo::Bool:          return "Bool";
  case mongo::Date:          return "Date";
  case mongo::jstNULL:       return "jstNULL";
  case mongo::RegEx:         return "RegEx";
  case mongo::DBRef:         return "DBRef";
  case mongo::Code:          return "Code";
  case mongo::Symbol:        return "Symbol";
  case mongo::CodeWScope:    return "CodeWScope";
  case mongo::NumberInt:     return "NumberInt";
  case mongo::Timestamp:     return "Timestamp";
  case mongo::NumberLong:    return "NumberLong";  // Same value as mongo::JSTypeMax
  case mongo::MaxKey:        return "MaxKey";
  }

  return "Unknown Mongo Type";
}
