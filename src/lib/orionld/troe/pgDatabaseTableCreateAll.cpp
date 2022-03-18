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

#include "orionld/common/pqHeader.h"                           // Postgres header
#include "orionld/troe/pgTransactionBegin.h"                   // pgTransactionBegin
#include "orionld/troe/pgTransactionRollback.h"                // pgTransactionRollback
#include "orionld/troe/pgTransactionCommit.h"                  // pgTransactionCommit
#include "orionld/troe/pgDatabaseTableCreateAll.h"             // Own interface



// -----------------------------------------------------------------------------
//
// The variable 'dbCreationCommand' comes from an auto-generated file:
//  src/lib/orionld/troe/dbCreationCommand.cpp
//
// No need to auto-generate also a header file - the variable is only used in this file.
//
extern const char* dbCreationCommand;



// -----------------------------------------------------------------------------
//
// pgDatabaseTableCreateAll -
//
// FIXME: The types ValueType+OperationMode needs some investigation
//        It seems like they should be created ONCE for all DBs.
//        If this is true, the creation of the types must be part of the postgres installation.
//        OR: pgInit() creates the types being prepared for failures of type "already exists"
//
bool pgDatabaseTableCreateAll(PGconn* connectionP)
{
  if (pgTransactionBegin(connectionP) == false)
    LM_RE(false, ("pgTransactionBegin failed"));

  PGresult* res = PQexec(connectionP, dbCreationCommand);
  if (res == NULL)
  {
    pgTransactionRollback(connectionP);
    LM_RE(false, ("Database Error (PQexec(%s): %s)", dbCreationCommand, PQresStatus(PQresultStatus(res))));
  }
  PQclear(res);

  pgTransactionCommit(connectionP);
  return true;
}
