/*
*
* Copyright 2020 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include </usr/pgsql-12/include/libpq-fe.h>                               // PGconn

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/PgConnection.h"                         // PgConnection
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgDatabaseCreate.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// pgDatabaseCreate - create a postgres database
//
// FIXME: the connection to the new database could be returned - avoid an extra close/connect
//
PgConnection* pgDatabaseCreate(PgConnection* nullConnectionP, const char* dbName)
{
  char          sql[512];
  PGresult*     res;
  PgConnection* dbConnectionP;

  //
  // Create the database (using an already established connection to the "NULL" database)
  //
  snprintf(sql, sizeof(sql), "CREATE DATABASE %s", dbName);
  res = PQexec(nullConnectionP->connectionP, sql);
  if (res == NULL)
    LM_RE(NULL, ("Database Error (PQexec(BEGIN): %s)", PQresStatus(PQresultStatus(res))));
  PQclear(res);


  //
  // Connect to the newly created database
  //
  dbConnectionP = pgConnectionGet(dbName);
  if ((dbConnectionP == NULL) || (dbConnectionP->connectionP == NULL))
    LM_RE(NULL, ("Database Error (unable to connect to postgres database '%s')", dbName));

#if 0
  //
  // Add extension 'timescaledb' to the database
  //
  res = PQexec(dbConnectionP->connectionP, "CREATE EXTENSION IF NOT EXISTS timescaledb");
  if (res == NULL)
  {
    PQclear(res);
    pgConnectionRelease(dbConnectionP);
    LM_RE(NULL, ("Database Error (Failing command: CREATE EXTENSION IF NOT EXISTS timescaledb)"));
  }
  PQclear(res);
#endif

  //
  // Add extension 'postgis' to the database
  //
  res = PQexec(dbConnectionP->connectionP, "CREATE EXTENSION IF NOT EXISTS postgis");
  if (res == NULL)
  {
    PQclear(res);
    pgConnectionRelease(dbConnectionP);
    LM_RE(NULL, ("Database Error (Failing command: CREATE EXTENSION IF NOT EXISTS postgis)"));
  }
  PQclear(res);

  return dbConnectionP;
}
