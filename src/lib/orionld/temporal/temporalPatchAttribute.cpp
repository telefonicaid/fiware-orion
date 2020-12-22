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
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/temporal/pgConnectionGet.h"                  // pgConnectionGet
#include "orionld/temporal/pgConnectionRelease.h"              // pgConnectionRelease
#include "orionld/temporal/pgTransactionBegin.h"               // pgTransactionBegin
#include "orionld/temporal/pgTransactionRollback.h"            // pgTransactionRollback
#include "orionld/temporal/pgTransactionCommit.h"              // pgTransactionCommit
#include "orionld/temporal/pgAttributeTreat.h"                 // pgAttyributeTreat
#include "orionld/temporal/temporalPatchAttribute.h"           // Own interface



// -----------------------------------------------------------------------------
//
// temporalSubAttrsExpand -
//
void temporalSubAttrsExpand(KjNode* treeP)
{
  for (KjNode* subAttrP = treeP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
  {
    if      (strcmp(subAttrP->name, "type")        == 0) {}
    else if (strcmp(subAttrP->name, "value")       == 0) {}
    else if (strcmp(subAttrP->name, "object")      == 0) {}
    else if (strcmp(subAttrP->name, "location")    == 0) {}
    else if (strcmp(subAttrP->name, "observedAt")  == 0) {}
    else if (strcmp(subAttrP->name, "unitCode")    == 0) {}
    else if (strcmp(subAttrP->name, "datasetId")   == 0) {}
    else
    {
      LM_TMP(("EXPAND: FROM '%s' (entity type value)", subAttrP->name));
      subAttrP->name = orionldContextItemExpand(orionldState.contextP, subAttrP->name, true, NULL);
      LM_TMP(("EXPAND: TO '%s'", subAttrP->name));
    }
  }
}



// ----------------------------------------------------------------------------
//
// temporalPatchAttribute -
//
// Input payload body is simply an attribute.
// The attribute's value must be pushed onto the 'attributes' table, with an opMode set to "REPLACE".
// Sub-attributes that have a NULL value are marked as DELETED.
// Updating the type of an attribute is not allowed - 'type' if present, is removed by the Service Routine.
//
bool temporalPatchAttribute(ConnectionInfo* ciP)
{
  // <DEBUG>
  char debugBuf[1024];
  kjRender(orionldState.kjsonP, orionldState.requestTree, debugBuf, sizeof(debugBuf));
  LM_TMP(("APPA: incoming tree: %s", debugBuf));
  // </DEBUG>

  // Expand names of sub-attributes - FIXME - let the Service Routine do this for us!
  temporalSubAttrsExpand(orionldState.requestTree);

  PGconn* connectionP = pgConnectionGet(dbName);
  if (connectionP == NULL)
    LM_RE(false, ("no connection to postgres"));

  if (pgTransactionBegin(connectionP) != true)
    LM_RE(false, ("pgTransactionBegin failed"));

  char* entityId       = orionldState.wildcard[0];
  char* attributeName  = orionldState.wildcard[1];

  //
  // pgAttributeTreat assumes the name of the attribute comes as part of the tree.
  // So, let's name the tree then! :)
  //
  orionldState.requestTree->name = orionldContextItemExpand(orionldState.contextP, attributeName, true, NULL);  // FIXME: unitCode, datasetId, ... no expansion!!!

  if (pgAttributeTreat(connectionP, orionldState.requestTree, NULL, entityId, orionldState.requestTimeString, orionldState.requestTimeString, TEMPORAL_ATTRIBUTE_UPDATE) == false)
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
