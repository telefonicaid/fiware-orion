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

#include <string>
#include <vector>

#include "mongoDriver/BSONElement.h"

namespace orion
{
/* ****************************************************************************
*
* BSONElement::BSONElement -
*/
BSONElement::BSONElement(void)
{
  // FIXME OLD-DR: really needed?
}



/* ****************************************************************************
*
* BSONElement::type -
*
*/
BSONType BSONElement::type(void) const
{
  switch (bv.value_type)
  {
  case BSON_TYPE_EOD:        return orion::EOO;
  case BSON_TYPE_DOUBLE:     return orion::NumberDouble;
  case BSON_TYPE_UTF8:       return orion::String;
  case BSON_TYPE_DOCUMENT:   return orion::Object;
  case BSON_TYPE_ARRAY:      return orion::Array;
  case BSON_TYPE_BINARY:     return orion::BinData;
  case BSON_TYPE_UNDEFINED:  return orion::Undefined;
  case BSON_TYPE_OID:        return orion::jstOID;
  case BSON_TYPE_BOOL:       return orion::Bool;
  case BSON_TYPE_DATE_TIME:  return orion::Date;
  case BSON_TYPE_NULL:       return orion::jstNULL;
  case BSON_TYPE_REGEX:      return orion::RegEx;
  case BSON_TYPE_DBPOINTER:  return orion::DBRef;
  case BSON_TYPE_CODE:       return orion::Code;
  case BSON_TYPE_SYMBOL:     return orion::Symbol;
  case BSON_TYPE_CODEWSCOPE: return orion::CodeWScope;
  case BSON_TYPE_INT32:      return orion::NumberInt;
  case BSON_TYPE_TIMESTAMP:  return orion::Timestamp;
  case BSON_TYPE_INT64:      return orion::NumberLong;
  case BSON_TYPE_DECIMAL128: return orion::BigDecimal;
  case BSON_TYPE_MAXKEY:     return orion::MaxKey;
  case BSON_TYPE_MINKEY:     return orion::MinKey;
  }

  // FIXME: maybe we should return some other thing...
  return orion::EOO;
}

// FIXME OLD-DR: type check should be enformed in Double(), String(), eg:
//
// value = bson_iter_value(&iter);
//
//if (value->value_type == BSON_TYPE_INT32) {
//   printf ("%d\n", value->value.v_int32);
//}


/* ****************************************************************************
*
* BSONElement::isNull -
*/
bool BSONElement::isNull(void)
{
  // FIXME OLD-DR: who calls this method?
  return (bv.value_type == BSON_TYPE_NULL);
}



/* ****************************************************************************
*
* BSONElement::OID -
*/
std::string BSONElement::OID(void)
{
  char str[25];  // OID fixed length is 24 chars
  bson_oid_to_string(&bv.value.v_oid, str);
  return std::string(str);
}



/* ****************************************************************************
*
* BSONElement::String -
*/
std::string BSONElement::String(void) const
{
  return std::string(bv.value.v_utf8.str);
}



/* ****************************************************************************
*
* BSONElement::Bool -
*/
bool BSONElement::Bool(void) const
{
  return bv.value.v_bool;
}


/* ****************************************************************************
*
* BSONElement::Number -
*/
double BSONElement::Number(void) const
{
  return bv.value.v_double;
}



/* ****************************************************************************
*
* BSONElement::Array -
*/
std::vector<BSONElement> BSONElement::Array(void) const
{
  std::vector<BSONElement> v;

  // First, get the bson_t corresponding to the array, from bv
  size_t len    = (size_t) bv.value.v_doc.data_len;
  uint8_t* data = bv.value.v_doc.data;

  bson_t* b = bson_new_from_buffer(&data, &len, NULL, NULL);

  // Next, iterate on the bson_t to build the array
  bson_iter_t iter;
  if (bson_iter_init(&iter, b))
  {
     while (bson_iter_next(&iter))
     {
        v.push_back(BSONElement(bson_iter_key(&iter), bson_iter_value(&iter)));
     }
  }

  // Free bson_t memory
  bson_destroy(b);

  return v;
}



/* ****************************************************************************
*
* BSONElement::embeddedObject -
*/
BSONObj BSONElement::embeddedObject(void) const
{
  size_t len    = (size_t) bv.value.v_doc.data_len;
  uint8_t* data = bv.value.v_doc.data;

  bson_t* b = bson_new_from_buffer(&data, &len, NULL, NULL);

  BSONObj bo(b);

  bson_destroy(b);

  return bo;
}



/* ****************************************************************************
*
* BSONElement::date -
*/
BSONDate BSONElement::date(void)
{
  return BSONDate(bv.value.v_datetime);
}



/* ****************************************************************************
*
* BSONElement::fieldName -
*/
std::string BSONElement::fieldName(void) const
{
  return field;
}



/* ****************************************************************************
*
* BSONElement::_str -
*/
std::string BSONElement::str() const
{
  // FIXME OLD-DR: probably this method can be removed. It's redundant. Who calls it?
  return String();
}



/* ****************************************************************************
*
* BSONElement::eoo -
*/
bool BSONElement::eoo(void) const
{
  // FIXME OLD-DR: who calls this method?
  return (bv.value_type == BSON_TYPE_NULL);
}

///////// from now on, only methods with low-level driver types in return or parameters /////////


/* ****************************************************************************
*
* BSONElement::BSONElement -
*/
BSONElement::BSONElement(const std::string& _field, const bson_value_t* _bv)
{
  field = _field;
  bv = *_bv;
}
}
