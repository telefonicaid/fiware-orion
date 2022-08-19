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

#include "mongoDriver/BSONArrayBuilder.h"

namespace orion
{
/* ****************************************************************************
*
* BSONArrayBuilder::BSONArrayBuilder -
*/
BSONArrayBuilder::BSONArrayBuilder(void)
{
  b = bson_new();
  size = 0;
}



/* ****************************************************************************
*
* BSONArrayBuilder::~BSONArrayBuilder -
*/
BSONArrayBuilder::~BSONArrayBuilder(void)
{
  bson_destroy(b);
}



/* ****************************************************************************
*
* BSONArrayBuilder::arr -
*/
BSONArray BSONArrayBuilder::arr(void)
{
  return BSONArray(b);
}



/* ****************************************************************************
*
* BSONArrayBuilder::arrSize -
*/
int BSONArrayBuilder::arrSize(void) const
{
  return size;
}



/* ****************************************************************************
*
* BSONArrayBuilder::append -
*/
void BSONArrayBuilder::append(const BSONObj& value)
{
  size_t keylen = bson_uint32_to_string(size++, &key, buf, sizeof buf);
  bson_append_document(b, key, (int) keylen, value.get());
}



/* ****************************************************************************
*
* BSONArrayBuilder::append -
*/
void BSONArrayBuilder::append(const BSONArray& value)
{
  size_t keylen = bson_uint32_to_string(size++, &key, buf, sizeof buf);
  bson_append_array(b, key, (int) keylen, value.get());
}



/* ****************************************************************************
*
* BSONArrayBuilder::append -
*/
void BSONArrayBuilder::append(const std::string& value)
{
  size_t keylen = bson_uint32_to_string(size++, &key, buf, sizeof buf);
  bson_append_utf8(b, key, (int) keylen, value.c_str(), -1);
}



/* ****************************************************************************
*
* BSONArrayBuilder::append -
*/
void BSONArrayBuilder::append(const char* value)
{
  size_t keylen = bson_uint32_to_string(size++, &key, buf, sizeof buf);
  bson_append_utf8(b, key, (int) keylen, value, -1);
}



/* ****************************************************************************
*
* BSONArrayBuilder::append -
*/
void BSONArrayBuilder::append(double value)
{
  size_t keylen = bson_uint32_to_string(size++, &key, buf, sizeof buf);
  bson_append_double(b, key, (int) keylen, value);
}



/* ****************************************************************************
*
* BSONArrayBuilder::append -
*/
void BSONArrayBuilder::append(bool value)
{
  size_t keylen = bson_uint32_to_string(size++, &key, buf, sizeof buf);
  bson_append_bool(b, key, (int) keylen, value);
}



/* ****************************************************************************
*
* BSONArrayBuilder::appendNull -
*/
void BSONArrayBuilder::appendNull(void)
{
  size_t keylen = bson_uint32_to_string(size++, &key, buf, sizeof buf);
  bson_append_null(b, key, (int) keylen);
}



/* ****************************************************************************
*
* BSONArrayBuilder::appendRegex -
*/
void BSONArrayBuilder::appendRegex(const std::string& value)
{
  size_t keylen = bson_uint32_to_string(size++, &key, buf, sizeof buf);
  bson_append_regex(b, key, (int) keylen, value.c_str(), NULL);
}


/* ****************************************************************************
*
* BSONArrayBuilder::BSONArrayBuilder= -
*/
BSONArrayBuilder& BSONArrayBuilder::operator= (const BSONArrayBuilder& rhs)
{
  // check not self-assignment
  if (this != &rhs)
  {
    // destroy existing b object, then copy rhs.b object
    bson_destroy(b);
    b = bson_copy(rhs.b);
    size = rhs.size;
  }
  return *this;
}
}
