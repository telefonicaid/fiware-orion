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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjChildRemove
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgTransactionBegin.h"                   // pgTransactionBegin
#include "orionld/troe/pgTransactionRollback.h"                // pgTransactionRollback
#include "orionld/troe/pgTransactionCommit.h"                  // pgTransactionCommit
#include "orionld/troe/pgEntityTreat.h"                        // pgEntityTreat
#include "orionld/troe/troePostEntities.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// troeEntityExpand -
//
void troeEntityExpand(KjNode* entityP)
{
  KjNode* attrP = entityP->value.firstChildP;
  KjNode* nextAttr;

  while (attrP != NULL)
  {
    nextAttr = attrP->next;

    if (strcmp(attrP->name, "type") == 0)
    {
      LM_TMP(("EXPAND: FROM '%s' (entity type value)", attrP->value.s));
      attrP->value.s = orionldContextItemExpand(orionldState.contextP, attrP->value.s, true, NULL);
      LM_TMP(("EXPAND: TO '%s' (entity type value)", attrP->value.s));
    }
    else if (strcmp(attrP->name, "id")               == 0) {}
    else if (strcmp(attrP->name, "location")         == 0) {}
    else if (strcmp(attrP->name, "observationSpace") == 0) {}
    else if (strcmp(attrP->name, "operationSpace")   == 0) {}
    else if ((strcmp(attrP->name, "createdAt") == 0) || (strcmp(attrP->name, "modifiedAt") == 0))
    {
      kjChildRemove(entityP, attrP);
    }
    else
    {
      attrP->name = orionldContextItemExpand(orionldState.contextP, attrP->name, true, NULL);

      if (attrP->type == KjObject)
      {
        KjNode* subAttrP = attrP->value.firstChildP;
        KjNode* nextSubAttr;

        while (subAttrP != NULL)
        {
          nextSubAttr = subAttrP->next;

          if      (strcmp(subAttrP->name, "type")        == 0) {}
          else if (strcmp(subAttrP->name, "id")          == 0) {}
          else if (strcmp(subAttrP->name, "value")       == 0) {}
          else if (strcmp(subAttrP->name, "object")      == 0) {}
          else if (strcmp(subAttrP->name, "observedAt")  == 0) {}
          else if (strcmp(subAttrP->name, "location")    == 0) {}
          else if (strcmp(subAttrP->name, "unitCode")    == 0) {}
          else if (strcmp(subAttrP->name, "datasetId")   == 0) {}
          else if ((strcmp(subAttrP->name, "createdAt") == 0) || (strcmp(subAttrP->name, "modifiedAt") == 0))
          {
            kjChildRemove(attrP, subAttrP);
          }
          else
          {
            LM_TMP(("EXPAND: FROM '%s'", subAttrP->name));
            subAttrP->name = orionldContextItemExpand(orionldState.contextP, subAttrP->name,  true, NULL);
            LM_TMP(("EXPAND: TO '%s'", subAttrP->name));
          }

          subAttrP = nextSubAttr;
        }
      }
    }

    attrP = nextAttr;
  }
}


// ----------------------------------------------------------------------------
//
// troePostEntities -
//
bool troePostEntities(ConnectionInfo* ciP)
{
  PGconn* connectionP;
  KjNode* entityP = orionldState.requestTree;

  //
  // FIXME: the tree should be served expanded by orionldPostEntities()
  //


  // Expand entity type and attribute names - FIXME: Remove once orionldPostEntities() has been fixed to do that
  troeEntityExpand(entityP);

  connectionP = pgConnectionGet(orionldState.troeDbName);
  if (connectionP == NULL)
    LM_RE(false, ("no connection to postgres"));

  if (pgTransactionBegin(connectionP) != true)
    LM_RE(false, ("pgTransactionBegin failed"));

  LM_TMP(("TEMP: Calling pgEntityTreat for entity at %p", entityP));
  char* entityId   = orionldState.payloadIdNode->value.s;
  char* entityType = orionldContextItemExpand(orionldState.contextP, orionldState.payloadTypeNode->value.s, true, NULL);
  if (pgEntityTreat(connectionP, entityP, entityId, entityType, TROE_ENTITY_CREATE, TROE_ENTITY_CREATE) == false)
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
