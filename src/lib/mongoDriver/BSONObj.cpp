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
#include <map>
#include <vector>
#include <set>

#include "mongoDriver/BSONObj.h"

#include "logMsg/logMsg.h"  // FIXME OLD-DR: remove after use

namespace orion
{
/* ****************************************************************************
*
* BSONObj::BSONObj -
*/
BSONObj::BSONObj()
{
  b = bson_new();
}



/* ****************************************************************************
*
* BSONObj::BSONObj -
*/
BSONObj::BSONObj(const BSONObj& _bo)
{
  b = bson_copy(_bo.b);
}



/* ****************************************************************************
*
* BSONObj::~BSONObj -
*/
BSONObj::~BSONObj(void)
{
  bson_destroy(b);
}



/* ****************************************************************************
*
* BSONObj::getFieldNames -
*/
int BSONObj::getFieldNames(std::set<std::string>* fields) const
{
  bson_iter_t iter;
  unsigned int n;

  if (bson_iter_init(&iter, b))
  {
     while (bson_iter_next(&iter))
     {
        n++;
        fields->insert(std::string(bson_iter_key(&iter)));
     }
     return n;
  }

  return -1;
}



/* ****************************************************************************
*
* BSONObj::hasField -
*/
bool BSONObj::hasField(const std::string& field) const
{
  return bson_has_field(b, field.c_str());
}



/* ****************************************************************************
*
* BSONObj::nFields -
*/
int BSONObj::nFields(void) const
{
  return bson_count_keys(b);
}



/* ****************************************************************************
*
* BSONObj::toString -
*/
std::string BSONObj::toString(void) const
{
  char* str = bson_as_relaxed_extended_json(b, NULL);
  std::string s(str);
  bson_free(str);
  return s;
}



/* ****************************************************************************
*
* BSONObj::isEmpty -
*/
bool BSONObj::isEmpty(void)
{
  return (bson_count_keys(b) == 0);
}



/* ****************************************************************************
*
* BSONObj::toStringMap -
*/
void BSONObj::toStringMap(std::map<std::string, std::string>* m)
{
  bson_iter_t iter;
  if (bson_iter_init(&iter, b))
  {
     while (bson_iter_next(&iter))
     {
       if (bson_iter_type(&iter) == BSON_TYPE_UTF8)
       {
         (*m)[bson_iter_key(&iter)] = bson_iter_utf8(&iter, NULL);
       }
       else
       {
         // FIXME OLD-DR: trace as fatal error
       }
     }
  }
}



/* ****************************************************************************
*
* BSONObj::toElementsVector -
*/
void BSONObj::toElementsVector(std::vector<BSONElement>* v)
{
  bson_iter_t iter;
  if (bson_iter_init(&iter, b))
  {
     while (bson_iter_next(&iter))
     {
        v->push_back(BSONElement(bson_iter_key(&iter), bson_iter_value(&iter)));
     }
  }
}



/* ****************************************************************************
*
* BSONObj::operator= -
*
* FIXME OLD-DR: we should try to use const BSONObj& as argument
*/
BSONObj& BSONObj::operator= (BSONObj rhs)
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
* BSONObj::BSONObj -
*
*/
BSONObj::BSONObj(const bson_t* _b)
{
  b = bson_copy(_b);
}



/* ****************************************************************************
*
* BSONObj::get -
*/
bson_t* BSONObj::get(void) const
{
  return b;
}
}
