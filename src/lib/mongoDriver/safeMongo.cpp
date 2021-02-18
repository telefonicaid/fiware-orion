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
* Author: Fermin Galan Marquez
*/

#include <string>
#include <vector>

#include "logMsg/logMsg.h"

#include "mongoDriver/safeMongo.h"



/* ****************************************************************************
*
* bsonTypeNumber -
*/
static int bsonTypeNumber(bson_t* b, const char* field)
{
  bson_iter_t iter;
  if (bson_iter_init_find(&iter, b, field))
  {
    return  bson_iter_type(&iter);
  }

  LM_E(("Runtime Error (error getting BSON type for field %s)", field));
  return -1;
}



/* ****************************************************************************
*
* logKeyNotFound -
*/
void static logKeyNotFound(const char* supposed, const char* field, bson_t* b, const char* caller, int line)
{
  char* bsonStr = bson_as_relaxed_extended_json(b, NULL);

  int type = bsonTypeNumber(b, field);

  LM_E(("Runtime Error (field '%s' was supposed to be %s but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
        field, supposed, orion::bsonType2String((orion::BSONType) type), field, type, bsonStr, caller, line));

  bson_free(bsonStr);
}



/* ****************************************************************************
*
* logWrongType -
*/
void static logWrongType(const char* supposed, const char* field, bson_t* b, const char* caller, int line)
{
  char* bsonStr = bson_as_relaxed_extended_json(b, NULL);

  LM_E(("Runtime Error (%s field '%s' is missing in BSONObj <%s> from caller %s:%d)",
        field,
        supposed,
        bsonStr,
        caller,
        line));

  bson_free(bsonStr);
}



/* ****************************************************************************
*
* logNotStringInArray -
*/
void static logNotStringInArray(int ix, const char* field, bson_type_t type, const char* caller, int line)
{
  LM_E(("Runtime Error (element %d in array '%s' was supposed to be a string but the type is '%s' (type as integer: %d) from caller %s:%d)",
        ix, field, orion::bsonType2String((orion::BSONType) type), type, caller, line));
}



/* ****************************************************************************
*
* orion::getObjectField -
*/
orion::BSONObj orion::getObjectField
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_DOCUMENT)
     {
       const bson_value_t* bv = bson_iter_value(&iter);

       size_t len    = (size_t) bv->value.v_doc.data_len;
       uint8_t* data = bv->value.v_doc.data;

       bson_t* doc = bson_new_from_buffer(&data, &len, NULL, NULL);
       orion::BSONObj r = orion::BSONObj(doc);

       bson_destroy(doc);

       return r;
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("an object", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("object", field.c_str(), b, caller.c_str(), line);
  }

  return orion::BSONObj();
}



/* ****************************************************************************
*
* orion::getArrayField -
*/
orion::BSONArray orion::getArrayField
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_ARRAY)
     {
       const bson_value_t* bv = bson_iter_value(&iter);

       size_t len    = (size_t) bv->value.v_doc.data_len;
       uint8_t* data = bv->value.v_doc.data;

       bson_t* doc = bson_new_from_buffer(&data, &len, NULL, NULL);
       orion::BSONArray r = orion::BSONArray(doc);

       bson_destroy(doc);

       return r;
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("an array", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("array", field.c_str(), b, caller.c_str(), line);
  }

  return orion::BSONArray();
}



/* ****************************************************************************
*
* orion::getStringField -
*/
std::string orion::getStringField
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_UTF8)
     {
       return std::string(bson_iter_utf8(&iter, NULL));
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("a string", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("string", field.c_str(), b, caller.c_str(), line);
  }

  return "";
}



/* ****************************************************************************
*
* orion::getNumberField -
*/
double orion::getNumberField
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_DOUBLE)
     {
       return bson_iter_double(&iter);
     }
     else if (bson_iter_type(&iter) == BSON_TYPE_INT32)
     {
       return bson_iter_int32(&iter);
     }
     else if (bson_iter_type(&iter) == BSON_TYPE_INT64)
     {
       return bson_iter_int64(&iter);
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("a number (double/int/long)", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("double/int/long", field.c_str(), b, caller.c_str(), line);
  }

  return -1;
}



/* ****************************************************************************
*
* orion::getIntField -
*/
int orion::getIntField
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_INT32)
     {
       return bson_iter_int32(&iter);
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("an int", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("int", field.c_str(), b, caller.c_str(), line);
  }

  return -1;
}



/* ****************************************************************************
*
* orion::getLongField -
*/
long long orion::getLongField
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_INT64)
     {
       return bson_iter_int64(&iter);
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("a long", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("long", field.c_str(), b, caller.c_str(), line);
  }

  return -1;
}



/* ****************************************************************************
*
* orion::getIntOrLongFieldAsLong -
*/
long long orion::getIntOrLongFieldAsLong
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_INT32)
     {
       return bson_iter_int32(&iter);
     }
     else if (bson_iter_type(&iter) == BSON_TYPE_INT64)
     {
       return bson_iter_int64(&iter);
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("int or long", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("int/long", field.c_str(), b, caller.c_str(), line);
  }

  return -1;
}



/* ****************************************************************************
*
* orion::getBoolField -
*/
bool orion::getBoolField
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_BOOL)
     {
       return bson_iter_bool(&iter);
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("a bool", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("bool", field.c_str(), b, caller.c_str(), line);
  }

  return false;
}



/* ****************************************************************************
*
* getField -
*/
orion::BSONElement orion::getField
(
  const orion::BSONObj&  _b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
    return orion::BSONElement(field, bson_iter_value(&iter));
  }
  else
  {
    // Key not found error
    logWrongType("", field.c_str(), b, caller.c_str(), line);
  }

  return orion::BSONElement();
}



/* ****************************************************************************
*
* setStringVector -
*/
void orion::setStringVector
(
  const orion::BSONObj&      _b,
  const std::string&         field,
  std::vector<std::string>*  v,
  const std::string&         caller,
  int                        line
)
{
  // Getting the "low level" driver objects
  bson_t* b = _b.get();

  bson_iter_t iter;
  bson_iter_t subiter;

  if (bson_iter_init(&iter, b) && bson_iter_find(&iter, field.c_str()))
  {
     if (bson_iter_type(&iter) == BSON_TYPE_ARRAY)
     {
       v->clear();
       bson_iter_recurse(&iter, &subiter);
       int ix = 0;
       while (bson_iter_next(&subiter))
       {
         if (bson_iter_type(&subiter) == BSON_TYPE_UTF8)
         {
           ix++;
           v->push_back(std::string(bson_iter_utf8(&subiter, NULL)));
         }
         else
         {
           // All elements in the array are supposed to be strings
           logNotStringInArray(ix, field.c_str(), bson_iter_type(&subiter), caller.c_str(), line);
           v->clear();
           return;
         }
       }
     }
     else
     {
       // Wrong type situation
       logKeyNotFound("an array", field.c_str(), b, caller.c_str(), line);
     }
  }
  else
  {
    // Key not found error
    logWrongType("array", field.c_str(), b, caller.c_str(), line);
  }
}
