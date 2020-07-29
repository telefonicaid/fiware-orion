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

#include "mongoDriver/DBCursor.h"

namespace orion
{
/* ****************************************************************************
*
* DBCursor::DBCursor -
*/
DBCursor::DBCursor()
{
}



/* ****************************************************************************
*
* DBCursor::isNull -
*
*/
bool DBCursor::isNull(void)
{
  return (dbc.get() == NULL);
}



/* ****************************************************************************
*
* DBCursor::more -
*
*/
bool DBCursor::more(void)
{
  return dbc->more();
}



/* ****************************************************************************
*
* DBCursor::more -
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



///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* DBCursor::set -
*/
void DBCursor::set(std::auto_ptr<mongo::DBClientCursor> _dbc)
{
  dbc = _dbc;
}
}
