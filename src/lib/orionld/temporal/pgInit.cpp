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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/temporal/pgDatabasePrepare.h"                // pgDatabasePrepare
#include "orionld/temporal/pgConnectionGet.h"                  // pgConnectionGet
#include "orionld/temporal/pgConnectionRelease.h"              // pgConnectionRelease
#include "orionld/temporal/pgInit.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// pgInit -
//
bool pgInit(const char* dbPrefix)
{
#if 0
  //
  // Create the default database with its tables unless it already exists
  //
  LM_TMP(("PGINIT: Calling pgDatabasePrepare(%s)", dbPrefix));
  if (pgDatabasePrepare(dbPrefix) == false)
    LM_RE(false, ("unable to prepare the default database '%s'", dbPrefix));
#else
  //
  // Make sure the database for the default tenant exists and is initialized with its tables
  //
  LM_TMP(("PGINIT: connecting to db '%s'", dbPrefix));
  PGconn* connectionP = pgConnectionGet(dbPrefix);
  LM_TMP(("PGINIT: connection: %p", connectionP));

  if (connectionP == NULL)
    LM_RE(false, ("Unable to connect to temporal database '%s'", dbPrefix));

  ConnStatusType status;
  if ((status = PQstatus(connectionP)) != CONNECTION_OK)
  {
    PQfinish(connectionP);
    LM_RE(false, ("Unable to connect to temporal database '%s': status: %d", dbPrefix, status));
  }

  LM_TMP(("PGINIT: releasing the db connection"));
  pgConnectionRelease(connectionP);
  LM_TMP(("PGINIT: all good"));
#endif

  return true;
}
