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
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgTransactionBegin.h"                   // pgTransactionBegin
#include "orionld/troe/pgTransactionRollback.h"                // pgTransactionRollback
#include "orionld/troe/pgTransactionCommit.h"                  // pgTransactionCommit
#include "orionld/troe/pgEntityTreat.h"                        // pgEntityTreat
#include "orionld/troe/troePostEntityNoOverwrite.h"            // troePostEntityNoOverwrite
#include "orionld/troe/troeSubAttrsExpand.h"                   // troeSubAttrsExpand
#include "orionld/troe/troePostEntity.h"                       // Own interface



// ----------------------------------------------------------------------------
//
// troePostEntity -
//
bool troePostEntity(ConnectionInfo* ciP)
{
  //
  // The service routine leaves us with the attributes expanded but the sub attributes NOT expanded
  //
  for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type == KjObject)      // Normal attribute
      troeSubAttrsExpand(attrP);
    else if (attrP->type == KjArray)  // An array with datasetId instances of the attribute
    {
      for (KjNode* attrInstanceP = attrP->value.firstChildP; attrInstanceP != NULL; attrInstanceP = attrInstanceP->next)
      {
        troeSubAttrsExpand(attrInstanceP);
      }
    }
  }

  if (orionldState.uriParamOptions.noOverwrite == true)
    return troePostEntityNoOverwrite(ciP);

  PGconn* connectionP = pgConnectionGet(orionldState.troeDbName);
  if (connectionP == NULL)
    LM_RE(false, ("no connection to postgres"));

  if (pgTransactionBegin(connectionP) != true)
    LM_RE(false, ("pgTransactionBegin failed"));

  char* entityId   = orionldState.wildcard[0];
  char* entityType = (char*) "REPLACE";
  LM_TMP(("TEMP: Calling pgEntityTreat for entity '%s'", entityId));
  if (pgEntityTreat(connectionP, orionldState.requestTree, entityId, entityType, TROE_ENTITY_UPDATE, TROE_ATTRIBUTE_REPLACE) == false)
  {
    LM_E(("Database Error (post entities TRoE layer failed)"));
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
