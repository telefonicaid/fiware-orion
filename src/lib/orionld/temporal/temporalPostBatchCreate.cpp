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
#include "kbase/kMacros.h"                                     // K_FT
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/HttpStatusCode.h"                               // SccNotImplemented
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/temporal/pgConnectionGet.h"                  // pgConnectionGet
#include "orionld/temporal/pgConnectionRelease.h"              // pgConnectionRelease
#include "orionld/temporal/pgTransactionBegin.h"               // pgTransactionBegin
#include "orionld/temporal/pgTransactionRollback.h"            // pgTransactionRollback
#include "orionld/temporal/pgTransactionCommit.h"              // pgTransactionCommit
#include "orionld/temporal/pgEntityTreat.h"                    // pgEntityTreat
#include "orionld/temporal/temporalPostEntities.h"             // Own interface



// -----------------------------------------------------------------------------
//
// temporalEntityArrayExpand -
//
void temporalEntityArrayExpand(KjNode* tree)
{
  for (KjNode* entityP = tree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    for (KjNode* nodeP = entityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (strcmp(nodeP->name, "type") == 0)
        nodeP->value.s = orionldContextItemExpand(orionldState.contextP, nodeP->value.s, true, NULL);
      else if (strcmp(nodeP->name, "id")       == 0) {}
      else if (strcmp(nodeP->name, "location") == 0) {}
      else
        nodeP->name = orionldContextItemExpand(orionldState.contextP, nodeP->name, true, NULL);
    }
  }
}


#define TEMP_TRANSACTIONS 1
// ----------------------------------------------------------------------------
//
// temporalPostBatchCreate -
//
bool temporalPostBatchCreate(ConnectionInfo* ciP)
{
  PGconn* connectionP;

  // Expanding entity types and attribute names
  // FIXME: the tree should be served expanded
  temporalEntityArrayExpand(orionldState.requestTree);

  // FIXME: Implement orionldState.dbName
  if ((orionldState.tenant != NULL) && (orionldState.tenant[0] != 0))
    LM_X(1, ("Tenants (%s) not supported for the temporal layer (to be fixed asap)", orionldState.tenant));

  connectionP = pgConnectionGet(dbName);
  if (connectionP == NULL)
    LM_RE(false, ("no connection to postgres"));

#ifdef TEMP_TRANSACTIONS
  if (pgTransactionBegin(connectionP) != true)
    LM_RE(false, ("pgTransactionBegin failed"));
#endif

  bool ok = true;
  for (KjNode* entityP = orionldState.requestTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    LM_TMP(("TEMP: Calling pgEntityTreat for entity at %p", entityP));
    if (pgEntityTreat(connectionP, entityP, NULL, NULL, orionldState.requestTimeString, orionldState.requestTimeString) == false)
    {
      ok = false;
      break;
    }
  }

  if (ok == false)
  {
    LM_E(("Database Error (batch create temporal layer failed)"));
    if (pgTransactionRollback(connectionP) == false)
      LM_RE(false, ("pgTransactionRollback failed"));
  }
  else
  {
    LM_TMP(("TEMP: Calling pgTransactionCommit"));
#ifdef TEMP_TRANSACTIONS
    if (pgTransactionCommit(connectionP) != true)
      LM_RE(false, ("pgTransactionCommit failed"));
#endif
  }

  LM_TMP(("TEMP: Calling pgConnectionRelease"));
  pgConnectionRelease(connectionP);
  LM_TMP(("TEMP: All OK"));
  return true;
}
