/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "mongoBackend/safeBsonGet.h"
#include "logMsg/logMsg.h"

using namespace mongo;

/* ****************************************************************************
*
* getObjectField -
*
*/
BSONObj getObjectField(const BSONObj& b, const std::string& field)
{
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (object field '%s' is missing in BSONObj <%s>)", field.c_str(), b.toString().c_str()));
    return BSONObj();
  }
  if (b.getField(field).type() != Object)
  {

    LM_E(("Runtime Error (field '%s' was supposed to be an object but type=%d in BSONObj <%s>)",
          field.c_str(), b.getField(field).type(), b.toString().c_str()));
    return BSONObj();
  }
  return b.getObjectField(field);
}

/* ****************************************************************************
*
* getStringField -
*
*/
std::string getStringField(const BSONObj& b, const std::string& field)
{
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (string field '%s' is missing in BSONObj <%s>)", field.c_str(), b.toString().c_str()));
    return "";
  }
  if (b.getField(field).type() != String)
  {
    LM_E(("Runtime Error (field '%s' was supposed to be a string but type=%d in BSONObj <%s>)",
          field.c_str(), b.getField(field).type(), b.toString().c_str()));
    return "";
  }
  return b.getStringField(field);
}

/* ****************************************************************************
*
* getIntField -
*
*/
int getIntField(const BSONObj& b, const std::string& field)
{
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (int field '%s' is missing in BSONObj <%s>)", field.c_str(), b.toString().c_str()));
    return -1;
  }
  if (b.getField(field).type() != NumberInt)
  {
    LM_E(("Runtime Error (field '%s' was supposed to be a int but type=%d in BSONObj <%s>)",
          field.c_str(), b.getField(field).type(), b.toString().c_str()));
    return -1;
  }
  return b.getIntField(field);
}

/* ****************************************************************************
*
* getBoolField -
*
*/
bool getBoolField(const BSONObj& b, const std::string& field)
{
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (bool field '%s' is missing in BSONObj <%s>)", field.c_str(), b.toString().c_str()));
    return -1;
  }
  if (b.getField(field).type() != Bool)
  {
    LM_E(("Runtime Error (field '%s' was supposed to be a bool but type=%d in BSONObj <%s>)",
          field.c_str(), b.getField(field).type(), b.toString().c_str()));
    return -1;
  }
  return b.getBoolField(field);
}

/* ****************************************************************************
*
* getField -
*
*/
BSONElement getField(const BSONObj& b, const std::string& field)
{
  if (!b.hasField(field))
  {
    LM_E(("Runtime Error (field '%s' is missing in BSONObj <%s>)", field.c_str(), b.toString().c_str()));
    return BSONElement();
  }
  return b.getField(field);
}

