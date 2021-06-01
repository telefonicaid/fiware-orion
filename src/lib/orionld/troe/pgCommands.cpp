/*
*
* Copyright 2021 FIWARE Foundation e.V.
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

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/PgConnection.h"                         // PgConnection
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgTransactionBegin.h"                   // pgTransactionBegin
#include "orionld/troe/pgTransactionRollback.h"                // pgTransactionRollback
#include "orionld/troe/pgTransactionCommit.h"                  // pgTransactionCommit
#include "orionld/troe/pgCommands.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// pgCommands -
//
void pgCommands(char* sql[], int commands)
{
  PgConnection* connectionP = pgConnectionGet(orionldState.troeDbName);

  if ((connectionP == NULL) || (connectionP->connectionP == NULL))
    LM_RVE(("no connection to postgres"));

  if (pgTransactionBegin(connectionP->connectionP) != true)
  {
    pgConnectionRelease(connectionP);
    LM_RVE(("pgTransactionBegin failed"));
  }

  for (int ix = 0; ix < commands; ix++)
  {
    // LM_TMP(("SQL: %s;", sql[ix]));
    PGresult* res = PQexec(connectionP->connectionP, sql[ix]);
    if (res == NULL)
    {
      LM_E(("Database Error (%s)", PQresStatus(PQresultStatus(res))));
      if (pgTransactionRollback(connectionP->connectionP) == false)
        LM_E(("Database Error (pgTransactionRollback failed too)"));
      pgConnectionRelease(connectionP);
      return;
    }
    PQclear(res);

    if (PQstatus(connectionP->connectionP) != CONNECTION_OK)
    {
      LM_E(("SQL[%p]: bad connection: %d", connectionP->connectionP, PQstatus(connectionP->connectionP)));  // FIXME: string! (last error?)
      if (pgTransactionRollback(connectionP->connectionP) == false)
        LM_E(("Database Error (pgTransactionRollback failed too)"));
      pgConnectionRelease(connectionP);
      return;
    }
  }

  if (pgTransactionCommit(connectionP->connectionP) != true)
    LM_E(("pgTransactionCommit failed"));

  pgConnectionRelease(connectionP);
}
