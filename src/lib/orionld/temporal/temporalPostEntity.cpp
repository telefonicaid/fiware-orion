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

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjRender.h"                                    // kjRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/temporal/pgConnectionGet.h"                  // pgConnectionGet
#include "orionld/temporal/pgConnectionRelease.h"              // pgConnectionRelease
#include "orionld/temporal/pgTransactionBegin.h"               // pgTransactionBegin
#include "orionld/temporal/pgTransactionRollback.h"            // pgTransactionRollback
#include "orionld/temporal/pgTransactionCommit.h"              // pgTransactionCommit
#include "orionld/temporal/pgEntityTreat.h"                    // pgEntityTreat
#include "orionld/temporal/temporalPostEntity.h"               // Own interface



// ----------------------------------------------------------------------------
//
// temporalPostEntity -
//
bool temporalPostEntity(ConnectionInfo* ciP)
{
  // <DEBUG>
  char debugBuf[1024];
  kjRender(orionldState.kjsonP, orionldState.requestTree, debugBuf, sizeof(debugBuf));
  LM_TMP(("APPA: incoming tree: %s", debugBuf));
  // </DEBUG>

#if 0
  LM_TMP(("APPA: incoming tree for Attribute Append|Ignore: %s", debugBuf));
  if (orionldState.uriParamOptions.noOverwrite == true)
    return temporalPostEntityNoOverwrite(ciP);
#endif

  LM_TMP(("APPA: incoming tree for Attribute Append|Replace: %s", debugBuf));
  // FIXME: Implement orionldState.dbName
  if ((orionldState.tenant != NULL) && (orionldState.tenant[0] != 0))
    LM_X(1, ("Tenants (%s) not supported for the temporal layer (to be fixed asap)", orionldState.tenant));

  PGconn* connectionP = pgConnectionGet(dbName);
  if (connectionP == NULL)
    LM_RE(false, ("no connection to postgres"));

  if (pgTransactionBegin(connectionP) != true)
    LM_RE(false, ("pgTransactionBegin failed"));

  char* entityId   = orionldState.wildcard[0];
  char* entityType = (char*) "REPLACE";
  LM_TMP(("TEMP: Calling pgEntityTreat for entity '%s'", entityId));
  if (pgEntityTreat(connectionP, orionldState.requestTree, entityId, entityType, orionldState.requestTimeString, orionldState.requestTimeString, TEMPORAL_ATTRIBUTE_REPLACE) == false)
  {
    LM_E(("Database Error (post entities temporal layer failed)"));
    if (pgTransactionRollback(connectionP) == false)
      LM_RE(false, ("pgTransactionRollback failed"));
  }
  else
  {
    if (pgTransactionCommit(connectionP) != true)
      LM_RE(false, ("pgTransactionCommit failed"));
  }

  pgConnectionRelease(connectionP);

  return true;
}
