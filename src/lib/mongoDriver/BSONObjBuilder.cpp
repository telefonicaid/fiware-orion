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

#include "mongoDriver/BSONObjBuilder.h"

// FIXME OLD-DR: general comment. Do we really need builder vs. no-builder classes or
// only just one family? What about using the bson_append_document_begin() and
// bson_append_document_end() methods (maybe performance is
// better: http://mongoc.org/libbson/current/bson_append_array_begin.html#description)?

namespace orion
{
/* ****************************************************************************
*
* BSONObjBuilder::BSONObjBuilder -
*/
BSONObjBuilder::BSONObjBuilder(void)
{
  b = bson_new();
}



/* ****************************************************************************
*
* BSONObjBuilder::~BSONObjBuilder -
*/
BSONObjBuilder::~BSONObjBuilder(void)
{
  bson_destroy(b);
}



/* ****************************************************************************
*
* BSONObjBuilder::obj -
*/
BSONObj BSONObjBuilder::obj(void)
{
  return BSONObj(b);
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, const std::string& value)
{
  BSON_APPEND_UTF8(b, key.c_str(), value.c_str());
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, const char* value)
{
  BSON_APPEND_UTF8(b, key.c_str(), value);
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, int value)
{
  // FIXME OLD-DR: or maybe BSON_APPEND_INT64 ?
  BSON_APPEND_INT32(b, key.c_str(), value);
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, long long value)
{
  // FIXME OLD-DR: or maybe BSON_APPEND_INT32 ?
  BSON_APPEND_INT64(b, key.c_str(), value);
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, double value)
{
  BSON_APPEND_DOUBLE(b, key.c_str(), value);
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, bool value)
{
  BSON_APPEND_BOOL(b, key.c_str(), value);
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, const orion::OID& value)
{
  const bson_oid_t v = value.get();
  BSON_APPEND_OID(b, key.c_str(), &v);
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, const BSONObj& value)
{
  BSON_APPEND_DOCUMENT(b, key.c_str(), value.get());
}



/* ****************************************************************************
*
* BSONObjBuilder::append -
*/
void BSONObjBuilder::append(const std::string& key, const BSONArray& value)
{
  BSON_APPEND_ARRAY(b, key.c_str(), value.get());
}



/* ****************************************************************************
*
* BSONObjBuilder::appendCode -
*/
void BSONObjBuilder::appendCode(const std::string& key, const std::string& value)
{
  BSON_APPEND_CODE(b, key.c_str(), value.c_str());
}



/* ****************************************************************************
*
* BSONObjBuilder::appendRegex -
*/
void BSONObjBuilder::appendRegex(const std::string& key, const std::string& value)
{
  BSON_APPEND_REGEX(b, key.c_str(), value.c_str(), NULL);
}



/* ****************************************************************************
*
* BSONObjBuilder::appendDate -
*/
void BSONObjBuilder::appendDate(const std::string& key, const BSONDate& value)
{
  BSON_APPEND_DATE_TIME(b, key.c_str(), value.get());
}



/* ****************************************************************************
*
* BSONObjBuilder::appendNull -
*/
void BSONObjBuilder::appendNull(const std::string& key)
{
  BSON_APPEND_NULL(b, key.c_str());
}



/* ****************************************************************************
*
* BSONObjBuilder::appendElements -
*/
void BSONObjBuilder::appendElements(orion::BSONObj _b)
{
  bson_iter_t iter;

  // Used in object and array cases
  size_t len;
  uint8_t* data;
  bson_t* doc;
  const bson_value_t* bv;

  if (bson_iter_init(&iter, _b.get()))
  {
     while (bson_iter_next(&iter))
     {
        const char* key = bson_iter_key(&iter);
        switch (bson_iter_type(&iter))
        {
        case BSON_TYPE_DOUBLE:
          append(key, bson_iter_double(&iter));
          continue;
        case BSON_TYPE_UTF8:
          append(key, bson_iter_utf8(&iter, NULL));
          continue;
        case BSON_TYPE_DOCUMENT:
          bv = bson_iter_value(&iter);

          len  = (size_t) bv->value.v_doc.data_len;
          data = bv->value.v_doc.data;

          doc = bson_new_from_buffer(&data, &len, NULL, NULL);

          BSON_APPEND_DOCUMENT(b, key, doc);

          bson_destroy(doc);

          continue;
        case BSON_TYPE_ARRAY:
          bv = bson_iter_value(&iter);

          len  = (size_t) bv->value.v_doc.data_len;
          data = bv->value.v_doc.data;

          doc = bson_new_from_buffer(&data, &len, NULL, NULL);

          BSON_APPEND_ARRAY(b, key, doc);

          bson_destroy(doc);

          continue;
        case BSON_TYPE_BOOL:
          append(key, bson_iter_bool(&iter));
          continue;
        case BSON_TYPE_DATE_TIME:
          BSON_APPEND_DATE_TIME(b, key, bson_iter_date_time(&iter));
          continue;
        case BSON_TYPE_REGEX:
          appendRegex(key, bson_iter_regex(&iter, NULL));
          continue;
        case BSON_TYPE_CODE:
          appendCode(key, bson_iter_regex(&iter, NULL));
          continue;
        case BSON_TYPE_INT32:
          append(key, (int) bson_iter_int32(&iter));
          continue;
        case BSON_TYPE_INT64:
          append(key, (long long) bson_iter_int64(&iter));
          continue;
        default:
          // FIXME OLD-DR: we should raise error here?
          continue;
        }
     }
  }
  else
  {
    // FIME OLD-DR: raise error?
  }
}



/* ****************************************************************************
*
* BSONObjBuilder::operator= -
*
* FIXME OLD-DR: we should try to use const BSONObjBuilder& as argument
*/
BSONObjBuilder& BSONObjBuilder::operator= (BSONObjBuilder rhs)
{
  // check not self-assignment
  if (this != &rhs)
  {
    // destroy existing b object, then copy rhs.b object
    bson_destroy(b);
    b = bson_copy(rhs.b);
  }
  return *this;
}
}
