/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "mongoBackend/MongoGlobal.h"
#include "logMsg/traceLevels.h"

#include "mongo/client/dbclient.h"

using namespace mongo;

/* ****************************************************************************
*
* query -
*
*/
bool query
(
    const std::string&             col,
    const BSONObj&                 q,
    std::auto_ptr<DBClientCursor>* cursor,
    std::string*                   err
)
{
  DBClientBase* connection = getMongoConnection();

  if (connection == NULL)
  {
     LM_E(("Fatal Error (null DB connection)"));
     *err = "null DB connection";
     return false;
  }

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'", col.c_str(), q.toString().c_str()));

  try
  {
    *cursor = connection->query(col.c_str(), q);

    // We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
    // raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
    // exception ourselves
    //
    if (cursor->get() == NULL)
    {
      throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    releaseMongoConnection(connection);
    LM_I(("Database Operation Successful (%s)", q.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);
    LM_E(("Database Error (%s)", e.what()));
    *err = "Database Error: " + std::string(e.what());
    return false;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    LM_E(("Database Error (generic exception)"));
    *err = "Database Error: generic exception";
    return false;
  }

  return true;
}
