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

#include "mongoDriver/BSONArray.h"

namespace orion
{
/* ****************************************************************************
*
* BSONArray::BSONArray -
*/
BSONArray::BSONArray()
{
  b = bson_new();
}



/* ****************************************************************************
*
* BSONArray::BSONArray -
*/
BSONArray::BSONArray(const BSONArray& _ba)
{
  b = bson_copy(_ba.get());
}



/* ****************************************************************************
*
* BSONArray::~BSONArray -
*/
BSONArray::~BSONArray(void)
{
  bson_destroy(b);
}



/* ****************************************************************************
*
* BSONArray::nFields -
*/
int BSONArray::nFields(void) const
{
  return bson_count_keys(b);
}



/* ****************************************************************************
*
* BSONArray::toString -
*/
std::string BSONArray::toString(void) const
{
  char* str = bson_array_as_json(b, NULL);
  std::string s(str);
  bson_free(str);
  return s;
}



/* ****************************************************************************
*
* BSONArray::operator= -
*/
BSONArray& BSONArray::operator= (const BSONArray& rhs)
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



///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* BSONArray::BSONArray -
*
*/
BSONArray::BSONArray(const bson_t* _b)
{
  b = bson_copy(_b);
}



/* ****************************************************************************
*
* BSONObj::get -
*/
bson_t* BSONArray::get(void) const
{
  return b;
}
}

