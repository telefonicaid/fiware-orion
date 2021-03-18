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

#include "mongoDriver/DBCursor.h"

namespace orion
{
/* ****************************************************************************
*
* DBCursor::DBCursor -
*/
DBCursor::DBCursor()
{
  c = NULL;
}



/* ****************************************************************************
*
* DBCursor::~DBCursor -
*/
DBCursor::~DBCursor()
{
  if (c != NULL)
  {
    mongoc_cursor_destroy(c);
  }
}



/* ****************************************************************************
*
* DBCursor::isNull -
*
*/
bool DBCursor::isNull(void) const
{
  return (c == NULL);
}



/* ****************************************************************************
*
* DBCursor::next-
*
*/
bool DBCursor::next(BSONObj* nextDoc, int* errTypeP, std::string* err)
{
  const bson_t* doc;

  int           errType = ON_NEXT_NO_ERROR;
  std::string   exErr;

  bool r = mongoc_cursor_next(c, &doc);

  if (r)
  {
    // According to http://mongoc.org/libmongoc/current/mongoc_cursor_next.html#lifecycle note we don't have
    // to do a bson_destroy(&doc)
    *nextDoc = BSONObj(doc);
  }

  bson_error_t error;
  if (mongoc_cursor_error(c, &error))
  {
    // We can't return the error 'as is', as it may contain forbidden characters.
    // So, we can just match the error and send a less descriptive text.
    //
    const char* invalidPolygon      = "Exterior shell of polygon is invalid";
    const char* sortError           = "next(): { $err: \"Executor error: OperationFailed Sort operation used more than the maximum";
    if (strncmp(error.message, invalidPolygon, strlen(invalidPolygon)) == 0)
    {
      exErr = invalidPolygon;
      errType = ON_NEXT_MANAGED_ERROR;
    }
    else if (strncmp(error.message, sortError, strlen(sortError)) == 0)
    {
      exErr = "Sort operation used more than the maximum RAM. "
              "You should create an index. "
              "Check the Database Administration section in Orion documentation.";
      errType = ON_NEXT_MANAGED_ERROR;
    }
    else
    {
      exErr = "Error at querying MongoDB: " + std::string(error.message);
      errType = ON_NEXT_UNMANAGED_ERROR;
    }
  }

  if (err != NULL)
  {
    *err = exErr;
  }
  if (errTypeP != NULL)
  {
    *errTypeP = errType;
  }

  return r;
}



///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* DBCursor::set -
*/
void DBCursor::set(mongoc_cursor_t* _c)
{
  c = _c;
}
}
