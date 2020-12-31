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

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgTransactionBegin.h"                   // pgTransactionBegin
#include "orionld/troe/pgTransactionRollback.h"                // pgTransactionRollback
#include "orionld/troe/pgTransactionCommit.h"                  // pgTransactionCommit
#include "orionld/troe/pgEntityDelete.h"                       // pgEntityDelete
#include "orionld/troe/troePostBatchDelete.h"                  // Own interface



// ----------------------------------------------------------------------------
//
// troePostBatchDelete -
//
bool troePostBatchDelete(ConnectionInfo* ciP)
{
  PGconn* connectionP = pgConnectionGet(dbName);
  if (connectionP == NULL)
    LM_RE(false, ("no connection to postgres"));

  if (pgTransactionBegin(connectionP) != true)
    LM_RE(false, ("pgTransactionBegin failed"));

  bool allGood = true;
  for (KjNode* entityIdP = orionldState.requestTree->value.firstChildP; entityIdP != NULL; entityIdP = entityIdP->next)
  {
    char  instanceId[64];
    uuidGenerate(instanceId);

    if (pgEntityDelete(connectionP, instanceId, entityIdP->value.s, orionldState.requestTimeString) == false)
    {
      LM_E(("Database Error (batch delete entities TRoE layer failed)"));
      allGood = false;
      break;
    }
  }

  if (allGood == true)
  {
    if (pgTransactionCommit(connectionP) != true)
      LM_RE(false, ("pgTransactionCommit failed"));
  }
  else
  {
    if (pgTransactionRollback(connectionP) == false)
      LM_RE(false, ("pgTransactionRollback failed"));
  }

  pgConnectionRelease(connectionP);

  return true;
}
