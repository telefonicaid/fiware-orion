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
bool DBCursor::isNull(void)
{
  return (c == NULL);
}



/* ****************************************************************************
*
* DBCursor::more -
*
*/
bool DBCursor::more(void)
{
  // FIXME OLD-DR: probably this is not a good implementation
  // see http://mongoc.org/libmongoc/1.17.3/mongoc_cursor_more.html#description.
  // but the recommended way with mongoc_cursor_next() is not idempotent as it
  // consumes elements.
  // Probably this function should be removed in client API (.h)
  return mongoc_cursor_more(c);
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
    *nextDoc = BSONObj(doc);
  }

  // FIXME OLD-DR: should bson_destroy(doc)? According to http://mongoc.org/libmongoc/1.17.3/mongoc_cursor_next.html#lifecycle
  // is emphemeral so it is not needed...

  bson_error_t error;
  if (mongoc_cursor_error(c, &error))
  {
    // We can't return the error 'as is', as it may contain forbidden characters.
    // So, we can just match the error and send a less descriptive text.
    //
    const char* invalidPolygon      = "Exterior shell of polygon is invalid";
    const char* sortError           = "nextSafe(): { $err: \"Executor error: OperationFailed Sort operation used more than the maximum";
    const char* defaultErrorString  = "Error at querying MongoDB";

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
      exErr = defaultErrorString;
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



#if 0
/* ****************************************************************************
*
* DBCursor::nextSafe -
*
*/
BSONObj DBCursor::nextSafe(int* errTypeP, std::string* err)
{
  mongo::BSONObj next;
  std::string    exErr;
  int            errType = NEXTSAFE_CODE_NO_ERROR;
  try
  {
    next = dbc->nextSafe();
  }
  catch (const mongo::AssertionException &e)
  {
    errType = NEXTSAFE_CODE_MONGO_EXCEPTION;

    // We can't return the error 'as is', as it may contain forbidden characters.
    // So, we can just match the error and send a less descriptive text.
    //
    const char* invalidPolygon      = "Exterior shell of polygon is invalid";
    const char* sortError           = "nextSafe(): { $err: \"Executor error: OperationFailed Sort operation used more than the maximum";
    const char* defaultErrorString  = "Error at querying MongoDB";

    if (strncmp(exErr.c_str(), invalidPolygon, strlen(invalidPolygon)) == 0)
    {
      exErr = invalidPolygon;
    }
    else if (strncmp(exErr.c_str(), sortError, strlen(sortError)) == 0)
    {
      exErr = "Sort operation used more than the maximum RAM. "
              "You should create an index. "
              "Check the Database Administration section in Orion documentation.";
    }
    else
    {
      exErr = defaultErrorString;
    }
  }
  catch (const std::exception &e)
  {
    errType = NEXTSAFE_CODE_NO_MONGO_EXCEPTION;
    exErr   = e.what();
  }
  catch (...)
  {
    errType = NEXTSAFE_CODE_NO_MONGO_EXCEPTION;
    exErr   = "generic exception at nextSafe()";
  }

  if (err != NULL)
  {
    *err = exErr;
  }
  if (errTypeP != NULL)
  {
    *errTypeP = errType;
  }
  return BSONObj(next);
}
#endif



///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* DBCursor::set -
*/
void DBCursor::set(mongoc_cursor_t* _c)
{
  // FIXME OLD-DR: is this safe? who is using this function?
  c = _c;
}
}
