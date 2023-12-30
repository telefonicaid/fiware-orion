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
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/PgTableDefinitions.h"                  // PG_ATTRIBUTE_INSERT_START, PG_SUB_ATTRIBUTE_INSERT_START
#include "orionld/types/PgAppendBuffer.h"                      // PgAppendBuffer
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/pgAppendInit.h"                         // pgAppendInit
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgEntityBuild.h"                        // pgEntityBuild
#include "orionld/troe/pgCommands.h"                           // pgCommands
#include "orionld/troe/troePostEntities.h"                     // Own interface



// ----------------------------------------------------------------------------
//
// troePostEntities -
//
bool troePostEntities(void)
{
  char*    entityId    = (orionldState.payloadIdNode   != NULL)? orionldState.payloadIdNode->value.s   : NULL;
  char*    entityType  = (orionldState.payloadTypeNode != NULL)? orionldState.payloadTypeNode->value.s : NULL;
  KjNode*  entityP     = orionldState.requestTree;

  if (entityId == NULL)
  {
    entityId = orionldState.wildcard[0];  // troePutEntity passes the Entity ID via wildcards

    if (entityId == NULL)
      LM_RE(false, ("No entity ID"));
  }

  //
  // orionldPostTemporalEntities/orionldPostEntities does all the expansion
  //

  PgAppendBuffer entities;
  PgAppendBuffer attributes;
  PgAppendBuffer subAttributes;

  pgAppendInit(&entities, 1024);         // Just a single entity - 1024 should be more than enough
  pgAppendInit(&attributes, 2*1024);     // 2k - enough only for smaller entities - will be reallocated if necessary
  pgAppendInit(&subAttributes, 2*1024);  // ditto

  pgAppend(&entities,      PG_ENTITY_INSERT_START,        0);
  pgAppend(&attributes,    PG_ATTRIBUTE_INSERT_START,     0);
  pgAppend(&subAttributes, PG_SUB_ATTRIBUTE_INSERT_START, 0);

  const char* opModeString = (orionldState.troeOpMode == TROE_ENTITY_CREATE)? "Create" : "Replace";

  pgEntityBuild(&entities, opModeString, entityP, entityId, entityType, &attributes, &subAttributes);

  char* sqlV[3];
  int   sqlIx = 0;

  if (entities.values      > 0) sqlV[sqlIx++] = entities.buf;
  if (attributes.values    > 0) sqlV[sqlIx++] = attributes.buf;
  if (subAttributes.values > 0) sqlV[sqlIx++] = subAttributes.buf;

  if (sqlIx > 0)
    pgCommands(sqlV, sqlIx);

  return true;
}
