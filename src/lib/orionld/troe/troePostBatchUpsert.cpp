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
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/troeIgnored.h"                        // troeIgnored
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgTransactionBegin.h"                   // pgTransactionBegin
#include "orionld/troe/pgTransactionRollback.h"                // pgTransactionRollback
#include "orionld/troe/pgTransactionCommit.h"                  // pgTransactionCommit
#include "orionld/troe/pgEntityTreat.h"                        // pgEntityTreat
#include "orionld/troe/troeEntityArrayExpand.h"                // troeEntityArrayExpand
#include "orionld/troe/troePostBatchUpsert.h"                  // Own interface



// ----------------------------------------------------------------------------
//
// entityIdLookup - find an entity id in an array of objects containing "id" as one member
//
static KjNode* entityIdLookup(KjNode* tree, const char* entityId)
{
  for (KjNode* itemP = tree->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    KjNode* idP = kjLookup(itemP, "id");

    if ((idP != NULL) && (strcmp(idP->value.s, entityId) == 0))
      return idP;
  }

  return NULL;
}



// ----------------------------------------------------------------------------
//
// troePostBatchUpsert -
//
bool troePostBatchUpsert(ConnectionInfo* ciP)
{
  PGconn* connectionP;

  LM_TMP(("TROE: orionldState.troeDbName: '%s'", orionldState.troeDbName));

  connectionP = pgConnectionGet(orionldState.troeDbName);
  if (connectionP == NULL)
    LM_RE(false, ("unable to connect to postgres DB '%s'", orionldState.tenant));
  LM_TMP(("TROE: connection OK"));

  if (pgTransactionBegin(connectionP) != true)
    LM_RE(false, ("pgTransactionBegin failed"));
  LM_TMP(("TROE: pgTransactionBegin OK"));

  bool     ok                 = true;
  TroeMode troeEntityMode     = TROE_ENTITY_REPLACE;
  TroeMode troeAttributeMode  = TROE_ATTRIBUTE_REPLACE;

  if (orionldState.duplicateArray != NULL)
  {
    troeEntityArrayExpand(orionldState.duplicateArray);  // FIXME: Remove once orionldPostBatchUpsert.cpp has been fixed to do this
    for (KjNode* entityP = orionldState.duplicateArray->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      if (pgEntityTreat(connectionP, entityP, NULL, NULL, troeEntityMode, troeAttributeMode) == false)
      {
        LM_E(("Database Error (pgEntityTreat failed)"));
        ok = false;
        break;
      }
    }
  }

  if (ok == true)
  {
    // Expanding entity types and attribute names - FIXME: Remove once orionldPostBatchUpsert.cpp has been fixed to do this
    troeEntityArrayExpand(orionldState.requestTree);

    for (KjNode* entityP = orionldState.requestTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      if (troeIgnored(entityP) == true)
        continue;

      KjNode* entityIdP = kjLookup(entityP, "id");
      if (entityIdP != NULL)  // Can't be NULL, really ...
      {
        if (orionldState.batchEntities != NULL)
        {
          // If the entity already existed, the entity op mode must be "REPLACE"
          if (entityIdLookup(orionldState.batchEntities, entityIdP->value.s) == NULL)
          {
            troeEntityMode    = TROE_ENTITY_CREATE;
            troeAttributeMode = TROE_ENTITY_CREATE;
          }
          else
          {
            troeEntityMode    = (orionldState.uriParamOptions.update == true)? TROE_ENTITY_UPDATE : TROE_ENTITY_REPLACE;
            troeAttributeMode = TROE_ATTRIBUTE_REPLACE;
          }
        }
        else
        {
          troeEntityMode    = TROE_ENTITY_CREATE;
          troeAttributeMode = TROE_ENTITY_CREATE;
        }
      }

      if (pgEntityTreat(connectionP, entityP, NULL, NULL, troeEntityMode, troeAttributeMode) == false)
      {
        LM_E(("Database Error (pgEntityTreat failed)"));
        ok = false;
        break;
      }
    }
  }

  if (ok == false)
  {
    LM_E(("Database Error (batch create TRoE layer failed)"));
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
