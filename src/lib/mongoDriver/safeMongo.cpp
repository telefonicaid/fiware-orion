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
#include "ngsi/SubscriptionId.h"
#include "ngsi/RegistrationId.h"
#include "ngsi/StatusCode.h"

#include "mongoDriver/safeMongo.h"


#if 0
/* ****************************************************************************
*
* mongoTypeName -
*/
static const char* mongoTypeName(mongo::BSONType type)
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
#endif

/* ****************************************************************************
*
* logKeyNotFound -
*/
void static logKeyNotFound(const char* supposed, const char* field, bson_t* b, const char* caller, int line)
{
  char* bsonStr = bson_as_relaxed_extended_json(b, NULL);

  LM_E(("Runtime Error (field '%s' was supposed to be %s but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
        supposed, field, "" /* mongoTypeName(b.getField(field).type()) */, 0 /*b.getField(field).type()*/, "", bsonStr, caller, line));

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
        supposed,
        field,
        bsonStr,
        caller,
        line));

  bson_free(bsonStr);
}



/* ****************************************************************************
*
* logNotStringInArray -
*/
void static logNotStringInArray(int ix, const char* caller, int line)
{
  // char* bsonStr = bson_as_relaxed_extended_json(b, NULL);

  LM_E(("Runtime Error (element %d in array was supposed to be a string but the type is '%s' (type as integer: %d) from caller %s:%d)",
        ix, "" /*mongoTypeName(ba[ix].type())*/, 0 /*ba[ix].type()*/, caller, line));

  // bson_free(bsonStr);
}



#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field) && b.getField(field).type() == mongo::Object)
  {
    // FIXME OLD-DR: try to make this as simpler as previous implemetnation, without using BSONObjBUilder
    return orion::BSONObj(b.getObjectField(field));
    /* orion::BSONObjBuilder bob;
    bob.appendElements(b.getObjectField(field));
    return bob.obj();*/
  }

  // Detect error
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (object field '%s' is missing in BSONObj <%s> from caller %s:%d)",
          field.c_str(),
          b.toString().c_str(),
          caller.c_str(),
          line));
  }
  else
  {
    LM_E(("Runtime Error (field '%s' was supposed to be an object but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
          field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str(), caller.c_str(), line));
  }
  return orion::BSONObj();
}
#endif


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


#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field) && b.getField(field).type() == mongo::Array)
  {
    // See http://stackoverflow.com/questions/36307126/getting-bsonarray-from-bsonelement-in-an-direct-way
    return orion::BSONArray((mongo::BSONArray) b.getObjectField(field));
  }

  // Detect error
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (object field '%s' is missing in BSONObj <%s> from caller %s:%d)",
          field.c_str(),
          b.toString().c_str(),
          caller.c_str(),
          line));
  }
  else
  {
    LM_E(("Runtime Error (field '%s' was supposed to be an array but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
          field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str(), caller.c_str(), line));
  }
  return orion::BSONArray();
}
#endif


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


#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field) && b.getField(field).type() == mongo::String)
  {
    return b.getStringField(field);
  }

  // Detect error
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (string field '%s' is missing in BSONObj <%s> from caller %s:%d)",
          field.c_str(),
          b.toString().c_str(),
          caller.c_str(),
          line));
  }
  else
  {
    LM_E(("Runtime Error (field '%s' was supposed to be a string but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
          field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str(), caller.c_str(), line));
  }

  return "";
}
#endif



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


#if 0
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
  double retVal = -1;

  // Getting the "low level" driver objects
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field))
  {
    if      (b.getField(field).type() == mongo::NumberDouble)      retVal = b.getField(field).Double();
    else if (b.getField(field).type() == mongo::NumberLong)        retVal = b.getField(field).Long();
    else if (b.getField(field).type() == mongo::NumberInt)         retVal = b.getField(field).Int();

    return retVal;
  }

  // Detect error
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (double/int/long field '%s' is missing in BSONObj <%s> from caller %s:%d)",
          field.c_str(),
          b.toString().c_str(),
          caller.c_str(),
          line));
  }
  else
  {
    LM_E(("Runtime Error (field '%s' was supposed to be a Number (double/int/long) but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
          field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str(), caller.c_str(), line));
  }

  return -1;
}
#endif


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



#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field) && b.getField(field).type() == mongo::NumberInt)
  {
    return b.getIntField(field);
  }

  // Detect error
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (int field '%s' is missing in BSONObj <%s> from caller %s:%d)",
          field.c_str(),
          b.toString().c_str(),
          caller.c_str(),
          line));
  }
  else
  {
    LM_E(("Runtime Error (field '%s' was supposed to be an int but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
          field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str(), caller.c_str(), line));
  }

  return -1;
}
#endif



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


#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field) && (b.getField(field).type() == mongo::NumberLong))
  {
    return b.getField(field).Long();
  }

  // Detect error
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (long field '%s' is missing in BSONObj <%s> from caller %s:%d)",
          field.c_str(),
          b.toString().c_str(),
          caller.c_str(),
          line));
  }
  else
  {
    LM_E(("Runtime Error (field '%s' was supposed to be a long but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
          field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str(), caller.c_str(), line));
  }

  return -1;
}
#endif



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


#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field))
  {
    if (b.getField(field).type() == mongo::NumberLong)
    {
      return b.getField(field).Long();
    }
    else if (b.getField(field).type() == mongo::NumberInt)
    {
      return b.getField(field).Int();
    }
  }

  // Detect error
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (int/long field '%s' is missing in BSONObj <%s> from caller %s:%d)",
          field.c_str(),
          b.toString().c_str(),
          caller.c_str(),
          line));
  }
  else
  {
    LM_E(("Runtime Error (field '%s' was supposed to be int or long but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
          field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str(), caller.c_str(), line));
  }

  return -1;
}
#endif

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


#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field) && b.getField(field).type() == mongo::Bool)
  {
    return b.getBoolField(field);
  }

  // Detect error
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (bool field '%s' is missing in BSONObj <%s> from caller %s:%d)",
          field.c_str(),
          b.toString().c_str(),
          caller.c_str(),
          line));
  }
  else
  {
    /*field.c_str();
    b.getField(field);
    b.getField(field).type();
    b.toString();
    b.toString().c_str();*/
    LM_E(("Runtime Error (field '%s' was supposed to be a bool but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
          field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str()));
  }

  return false;
}
#endif



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


#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field))
  {
    return orion::BSONElement(b.getField(field));
  }

  LM_E(("Runtime Error (field '%s' is missing in BSONObj <%s> from caller %s:%d)",
        field.c_str(),
        b.toString().c_str(),
        caller.c_str(),
        line));

  return orion::BSONElement();
}
#endif



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


#if 0
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
  const mongo::BSONObj b = _b.get();

  if (b.hasField(field) && b.getField(field).type() == mongo::Array)
  {
    // See http://stackoverflow.com/questions/36307126/getting-bsonarray-from-bsonelement-in-an-direct-way
    std::vector<mongo::BSONElement> ba = b.getField(field).Array();

    v->clear();

    for (unsigned int ix = 0; ix < ba.size(); ++ix)
    {
      if (ba[ix].type() == mongo::String)
      {
        v->push_back(ba[ix].String());
      }
      else
      {
        LM_E(("Runtime Error (element %d in array was supposed to be a string but the type is '%s' (type as integer: %d) from caller %s:%d)",
              ix, mongoTypeName(ba[ix].type()), ba[ix].type(), caller.c_str(), line));
        v->clear();

        return;
      }
    }
  }
  else
  {
    // Detect error
    if (!b.hasField(field))
    {
      LM_E(("Runtime Error (object field '%s' is missing in BSONObj <%s> from caller %s:%d)",
            field.c_str(),
            b.toString().c_str(),
            caller.c_str(),
            line));
    }
    else
    {
      LM_E(("Runtime Error (field '%s' was supposed to be an array but the type is '%s' (type as integer: %d) in BSONObj <%s> from caller %s:%d)",
            field.c_str(), "" /* mongoTypeName(b.getField(field).type()) */, "" /*b.getField(field).type()*/, b.toString().c_str(), caller.c_str(), line));
    }
  }
}
#endif



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
           logNotStringInArray(ix, caller.c_str(), line);
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


#if 0
/* ****************************************************************************
*
* orion::moreSafe -
*
* This is a safe version of the more() method for cursors in order to avoid
* exception that may crash the broker. However, if the more() method is returning
* an exception something really bad is happening, it is considered a Fatal Error.
*
* (The name resembles the same relationship between next() and nextSafe() in the mongo driver)
*
* FIXME OLD-DR: cursor.more() cannot raise exections... this method is probably useless
*/
bool orion::moreSafe(orion::DBCursor* cursor)
{
  try
  {
    return cursor->more();
  }
  catch (const std::exception& e)
  {
    LM_E(("Fatal Error (more() exception: %s)", e.what()));
    return false;
  }
  catch (...)
  {
    LM_E(("Fatal Error (more() exception: generic)"));
    return false;
  }
}
#endif


#if 0
/* ****************************************************************************
*
* orion::nextSafeOrError -
*/
bool orion::nextSafeOrError
(
  orion::DBCursor&    cursor,
  orion::BSONObj*     r,
  std::string*        err,
  const std::string&  caller,
  int                 line
)
{
  try
  {
    *r = cursor.nextSafe();
    return true;
  }
  catch (const std::exception &e)
  {
    char lineString[STRING_SIZE_FOR_INT];

    snprintf(lineString, sizeof(lineString), "%d", line);
    *err = std::string(e.what()) + " at " + caller + ":" + lineString;

    return false;
  }
  catch (...)
  {
    char lineString[STRING_SIZE_FOR_INT];

    snprintf(lineString, sizeof(lineString), "%d", line);
    *err = "generic exception at " + caller + ":" + lineString;

    return false;
  }
}
#endif


#if 0
/* ****************************************************************************
*
* orion::nextSafeOrError -
*
* FIXME OLD-DR: rename nextSafeOrError() -> nextOrError() ?
*/
bool orion::nextSafeOrError
(
  orion::DBCursor&    cursor,
  orion::BSONObj*     r,
  std::string*        err,
  const std::string&  caller,
  int                 line
)
{
  int errorType;
  if (!cursor.next(r, &errorType, err))
  {
    // Note that errorType doesn't progress upwards, as we only can provide the err string
    char lineString[STRING_SIZE_FOR_INT];
    snprintf(lineString, sizeof(lineString), "%d", line);
    *err = *err + " at " + caller + ":" + lineString;
    return false;
  }
  return true;
}
#endif


# if 0
/* ****************************************************************************
*
* orion::safeGetSubId -
*/
bool orion::safeGetSubId(const SubscriptionId& subId, orion::OID* id, StatusCode* sc)
{
  try
  {
    *id = orion::OID(mongo::OID(subId.get()));
    return true;
  }
  catch (const mongo::AssertionException &e)
  {
    // FIXME P3: this check is "defensive", but from a efficiency perspective this should be short-cut at
    // parsing stage. To check it. The check should be for: [0-9a-fA-F]{24}.
    sc->fill(SccContextElementNotFound);
    return false;
  }
  catch (const std::exception &e)
  {
    sc->fill(SccReceiverInternalError, e.what());
    return false;
  }
  catch (...)
  {
    sc->fill(SccReceiverInternalError, "generic exception");
    return false;
  }
}



/* ****************************************************************************
*
* orion::safeGetRegId -
*/
bool orion::safeGetRegId(const RegistrationId& regId, orion::OID* id, StatusCode* sc)
{
  try
  {
    *id = orion::OID(mongo::OID(regId.get()));
    return true;
  }
  catch (const mongo::AssertionException &e)
  {
    //
    // FIXME P3: this check is "defensive", but from a efficiency perspective this should be short-cut at
    // parsing stage. To check it. The check should be for: [0-9a-fA-F]{24}.
    //
    sc->fill(SccContextElementNotFound);
    return false;
  }
  catch (const std::exception &e)
  {
    sc->fill(SccReceiverInternalError, e.what());
    return false;
  }
  catch (...)
  {
    sc->fill(SccReceiverInternalError, "generic exception");
    return false;
  }
}
#endif


/* ****************************************************************************
*
* orion::safeGetSubId -
*
* FIXME OLD-DR: probably safeGetSubId and safeGetRegId could be refactored to unify
* FIXME OLD-DR: sc parameter is useless?
*/
bool orion::safeGetSubId(const SubscriptionId& subId, orion::OID* id, StatusCode* sc)
{
  *id = orion::OID(subId.get());
  return true;
}



/* ****************************************************************************
*
*  orion::safeGetRegId -
*
* FIXME OLD-DR: probably safeGetSubId and safeGetRegId could be refactored to unify
* FIXME OLD-DR: sc parameter is useless?
*/
bool orion::safeGetRegId(const RegistrationId& regId, orion::OID* id, StatusCode* sc)
{
  *id = orion::OID(regId.get());
  return true;
}
