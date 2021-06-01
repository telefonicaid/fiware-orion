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
  // Connect to the "NULL" database
  PgConnection*   nullConnectionP = pgConnectionGet(NULL);
  ConnStatusType  status          = ((nullConnectionP != NULL) && (nullConnectionP->connectionP != NULL))? PQstatus(nullConnectionP->connectionP) : CONNECTION_BAD;

  if (status != CONNECTION_OK)
    LM_RE(false, ("Database Error (unable to connect to postgres - connection/status: %p/%d)", nullConnectionP->connectionP, status));


  //
  // DB-Creation fails if the database already exists - that's OK
  //
  PgConnection* connectionP = pgDatabaseCreate(nullConnectionP, dbName);
  if (connectionP == NULL)
  {
    pgConnectionRelease(nullConnectionP);
    return true;
  }

  // Release the connection for the "NULL" DB
  pgConnectionRelease(nullConnectionP);

  bool r;
  if ((r = pgDatabaseTableCreateAll(connectionP->connectionP)) == false)
    LM_E(("Database Error (error creating postgres database tables)"));

  pgConnectionRelease(connectionP);

  return r;
}
