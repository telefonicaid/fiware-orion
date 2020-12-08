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
#include "orionld/temporal/pgConnectionGet.h"                  // Own interface


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
// For now we open and close the connection every time - this is not very gopod for performance, of course ...
//
PGconn* pgConnectionGet(const char* dbName)
{
  // FIXME: connection pool: lookup a free connection to 'dbName' ...
  char port[16];

  snprintf(port, sizeof(port), "%d", troePort);

  const char*  keywords[]   = { "host",   "port",   "user",   "password",  "dbname", NULL };
  const char*  values[]     = { troeHost, port,     troeUser, troePwd,     dbName,   NULL };
  PGconn*      connectionP  = PQconnectdbParams(keywords, values, 0);  // 0: no expansion of dbname - see https://www.postgresql.org/docs/12/libpq-connect.html

  if (connectionP == NULL)
    LM_RE(NULL, ("Database Error (unable  to connect to postgres('%s', %d, '%s', '%s', '%s')", troeHost, troePort, troeUser, troePwd, dbName));

  return connectionP;
}
