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

#include "orionld/common/orionldState.h"                       // dbName
#include "orionld/temporal/pgConnectionGet.h"                  // pgConnectionGet
#include "orionld/temporal/pgDatabaseCreate.h"                 // pgDatabaseCreate
#include "orionld/temporal/pgConnectionRelease.h"              // pgConnectionRelease
#include "orionld/temporal/pgDatabaseTableExists.h"            // pgDatabaseTableExists
#include "orionld/temporal/pgDatabaseTableCreateAll.h"         // pgDatabaseTableCreateAll
#include "orionld/temporal/pgDatabasePrepare.h"                // Own interface



// -----------------------------------------------------------------------------
//
// pgDatabasePrepare - prepare a postgres database (incl. creation if necessary)
//
// Create a postgres database named 'tenant', if it doesn't already exist
// 1. Try to connect to the db 'tenant'
// 2. If connection error AND error code == XXX, create the db 'tenant'
// 3. Make sure the three tables are there (test if 'tenant' already existed)
// 4. If tables not present, create them
//
bool pgDatabasePrepare(const char* db)
{
  LM_TMP(("PGINIT: Try-Connecting to '%s', and if that works, the DB already exists", db));
  bool    dbCreated   = false;
  PGconn* connectionP = pgConnectionGet(db);

  LM_TMP(("PGINIT: connectionP at %p", connectionP));
  if (connectionP == NULL)
  {
    LM_TMP(("PGINIT: Connection failed so, the db '%s' seems not to exist - creating it", db));

    connectionP = pgConnectionGet(dbName);  // FIXME: better name for the dbPrefix !!!

    if (connectionP == NULL)
      LM_RE(false, ("Database Error (error connecting to postgres db '%s')", dbName));

    if (pgDatabaseCreate(connectionP, db) == false)
      LM_RE(false, ("Database Error (error creating postgres db '%s')", db));

    pgConnectionRelease(connectionP);
    connectionP = pgConnectionGet(db);
    dbCreated = true;
  }

  if ((dbCreated == true) || (pgDatabaseTableExists(connectionP, "entities") == false))
  {
    if (pgDatabaseTableCreateAll(connectionP) == false)
      LM_RE(false, ("Database Error (error creating postgres database tables)"));
  }

  return true;
}
