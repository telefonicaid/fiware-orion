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

#include "orionld/common/orionldState.h"                       // troeHost, pgPortString, troeUser, troePwd
#include "orionld/troe/pgConnect.h"                            // Own interface



// -----------------------------------------------------------------------------
//
// pgConnect - connect to a postgres database
//
PGconn* pgConnect(const char* db)
{
  PGconn*  connectionP;
  int      attemptNo   = 0;
  int      maxAttempts = 100;
  char*    keywords[6] = { (char*) "host",   (char*) "port",       (char*) "user",   (char*) "password",  NULL, NULL };
  char*    values[6]   = { troeHost,         pgPortString,         troeUser,         troePwd,             NULL, NULL };

  if (db != NULL)
  {
    keywords[4] = (char*) "dbname";
    values[4]   = (char*) db;
  }

  while (attemptNo < maxAttempts)
  {
    connectionP  = PQconnectdbParams(keywords, values, 0);  // 0: no expansion of dbname - see https://www.postgresql.org/docs/12/libpq-connect.html

    ++attemptNo;
    if (connectionP != NULL)
      break;

    usleep(500);  // Sleep half a millisecond before we try again
    LM_W(("Unable to connect to postgres database '%s'", db));
  }

  if (connectionP == NULL)
    LM_RE(NULL, ("Database Error (unable to connect to postgres(host:'%s', port:%s, user:'%s', pwd:'%s', db:'%s')",
                 troeHost, pgPortString, troeUser, troePwd, db));

  return connectionP;
}
