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
#include <postgresql/libpq-fe.h>                               // PGconn

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/troe/pgDatabaseTableExists.h"                // Own interface



// -----------------------------------------------------------------------------
//
// pgDatabaseTableExists
//
bool pgDatabaseTableExists(PGconn* connectionP, const char* dbName, const char* tableName)
{
  char           sql[128];
  PGresult*      res;
  ExecStatusType status;

  snprintf(sql, sizeof(sql), "\\dt %s", tableName);
  res    = PQexec(connectionP, sql);
  status = PQresultStatus(res);
  if ((res == NULL) || (status != PGRES_TUPLES_OK))
    LM_RE(false, ("Database Error (unable to query Postgres db '%s' for the existence of the table '%s': %s)", dbName, tableName, PQresStatus(status)));
  PQclear(res);

  //
  // Get the response, parse it and make sure the table exists!
  //
  return true;
}
