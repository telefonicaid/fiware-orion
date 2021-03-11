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
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgDatabaseCreate.h"                     // pgDatabaseCreate
#include "orionld/troe/pgDatabaseTableCreateAll.h"             // pgDatabaseTableCreateAll
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgDatabasePrepare.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// pgDatabasePrepare - prepare a postgres database (incl. creation if necessary)
//
bool pgDatabasePrepare(const char* dbName)
{
  LM_TMP(("TROE: new tenant '%s'", dbName));

  // Connect to the "NULL" database
  PGconn*         connectionP = pgConnectionGet(NULL);
  ConnStatusType  status      = (connectionP != NULL)? PQstatus(connectionP) : CONNECTION_BAD;

  if (status != CONNECTION_OK)
    LM_RE(false, ("Database Error (unable to connect to postgres - connection / status: %p / %d)", connectionP, status));

  LM_TMP(("TROE: creating db '%s'", dbName));

  //
  // For now, we just create the Postgres database for the default tenant
  // This will fail if the database already exists - that's OK
  LM_TMP(("TROE: creating db '%s'", dbName));
  if (pgDatabaseCreate(connectionP, dbName) == false)  // FIXME: return the connection to the new DB ?
  {
    LM_TMP(("TROE: database '%s' seems to exist already", dbName));
    pgConnectionRelease(connectionP);
    return true;
  }
  LM_TMP(("TROE: db '%s' has been created", dbName));

  // Disconnect from the "NULL" DB
  pgConnectionRelease(connectionP);

  // Connect to the newly created database
  LM_TMP(("TROE: connecting to db '%s' to create the tables", dbName));

  connectionP = pgConnectionGet(dbName);
  if (connectionP == NULL)
    LM_RE(false, ("unable to connect to newly created postgres db '%s'", dbName));

  bool r;
  if ((r = pgDatabaseTableCreateAll(connectionP)) == false)
    LM_E(("Database Error (error creating postgres database tables)"));
  LM_TMP(("TROE: created tables for db '%s'", dbName));

  pgConnectionRelease(connectionP);

  return r;
}
