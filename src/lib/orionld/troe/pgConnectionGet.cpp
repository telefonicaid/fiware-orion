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
#include <postgresql/libpq-fe.h>                               // Postgres

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // troeHost, troePort, troeUser, troePwd
#include "orionld/troe/pgConnectionGet.h"                      // Own interface


//
// FIXME:
//   typedef struct PgConnection
//   {
//     sem_t    sem;
//     bool     taken;
//     PGconn*  connectionP;
//     char*    dbName;
//   } PgConnection;
//
// The connection pool will be simply a linked list of PgConnection.
// If none is free (for the dbName in question), another connection is opened.
//


// -----------------------------------------------------------------------------
//
// pgConnectionGet - get a connection to a postgres database
//
// FIXME: use a connection pool
//
// For now we open and close the connection every time - this is not very good for performance, of course ...
//
PGconn* pgConnectionGet(const char* db)
{
  // Empty tenant => use default db name
  if ((db != NULL) && (*db == 0))
    db = dbName;

  //
  // FIXME: connection pool: lookup a free connection to 'db' ...
  //
  // 1. PgDb* pgDbP = pgDatabaseLookup(db)
  // 2. if (pgDbP == NULL)
  //    {
  // 3.   pgDbSemTake();
  // 4.   pgDbP = pgDatabaseLookup(db);
  // 5.   if (pgDbP == NULL)
  // 6.     pgDbP = pgDatabaseAdd(db);
  //    }
  // 7.

  char     port[16];
  PGconn*  connectionP;
  snprintf(port, sizeof(port), "%d", troePort);

  int attemptNo   = 0;
  int maxAttempts = 100;

  while (attemptNo < maxAttempts)
  {
    if (db != NULL)
    {
      const char*  keywords[]   = { "host",   "port",   "user",   "password",  "dbname", NULL };
      const char*  values[]     = { troeHost, port,     troeUser, troePwd,     db,       NULL };

      connectionP  = PQconnectdbParams(keywords, values, 0);  // 0: no expansion of dbname - see https://www.postgresql.org/docs/12/libpq-connect.html
    }
    else
    {
      const char*  keywords[]   = { "host",   "port",   "user",   "password",  NULL };
      const char*  values[]     = { troeHost, port,     troeUser, troePwd,     NULL };

      connectionP  = PQconnectdbParams(keywords, values, 0);  // 0: no expansion of dbname - see https://www.postgresql.org/docs/12/libpq-connect.html
    }

    ++attemptNo;
    if (connectionP != NULL)
      break;

    usleep(100000);  // Sleep 0.1 seconds before we try again
  }

  if (connectionP == NULL)
    LM_RE(NULL, ("Database Error (unable  to connect to postgres('%s', %d, '%s', '%s', '%s')", troeHost, troePort, troeUser, troePwd, db));

  LM_TMP(("TROE: connected to db '%s', at 0x%x (on connection attempt %d)", db, connectionP, attemptNo));
  return connectionP;
}
