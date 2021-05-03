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
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/troeIgnored.h"                        // troeIgnored

#include "orionld/troe/troeEntityArrayExpand.h"                // troeEntityArrayExpand
#include "orionld/troe/PgTableDefinitions.h"                   // PG_ATTRIBUTE_INSERT_START, PG_SUB_ATTRIBUTE_INSERT_START
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppendInit.h"                         // pgAppendInit
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgAttributesBuild.h"                    // pgAttributesBuild
#include "orionld/troe/pgEntityBuild.h"                        // pgEntityBuild
#include "orionld/troe/pgCommands.h"                           // pgCommands
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
  PgAppendBuffer  entities;
  PgAppendBuffer  attributes;
  PgAppendBuffer  subAttributes;
  char*           troeEntityMode     = (char*) "Replace";

  pgAppendInit(&entities, 4*1024);       // 4k - will be reallocated if necessary
  pgAppendInit(&attributes, 8*1024);     // 8k - will be reallocated if necessary
  pgAppendInit(&subAttributes, 8*1024);  // ditto

  pgAppend(&entities,      PG_ENTITY_INSERT_START,        0);
  pgAppend(&attributes,    PG_ATTRIBUTE_INSERT_START,     0);
  pgAppend(&subAttributes, PG_SUB_ATTRIBUTE_INSERT_START, 0);

  troeEntityArrayExpand(orionldState.requestTree);

  if (orionldState.duplicateArray != NULL)
  {
    troeEntityArrayExpand(orionldState.duplicateArray);
    for (KjNode* entityP = orionldState.duplicateArray->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      pgEntityBuild(&entities, "Replace", entityP, NULL, NULL, &attributes, &subAttributes);
    }
  }

  troeEntityArrayExpand(orionldState.requestTree);
  for (KjNode* entityP = orionldState.requestTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    if (troeIgnored(entityP) == true)
      continue;

    KjNode* entityIdP    = kjLookup(entityP, "id");
    bool    entityUpdate = false;  // Updated of entities aren't recorded in the "entities" table

    if (entityIdP != NULL)  // Can't be NULL, really ...
    {
      if (orionldState.batchEntities != NULL)
      {
        // If the entity already existed, the entity op mode must be "REPLACE"
        if (entityIdLookup(orionldState.batchEntities, entityIdP->value.s) == NULL)
          troeEntityMode    = (char*) "Create";
        else
        {
          if (orionldState.uriParamOptions.update == true)
            entityUpdate = true;
          else
            troeEntityMode = (char*) "Replace";
        }
      }
      else
        troeEntityMode = (char*) "Create";
    }

    if (entityUpdate == true)
      pgAttributesBuild(&attributes, entityP, NULL, "Replace", &subAttributes);
    else
      pgEntityBuild(&entities, troeEntityMode, entityP, NULL, NULL, &attributes, &subAttributes);
  }


  char* sqlV[3];
  int   sqlIx = 0;

  if (entities.values      > 0) sqlV[sqlIx++] = entities.buf;
  if (attributes.values    > 0) sqlV[sqlIx++] = attributes.buf;
  if (subAttributes.values > 0) sqlV[sqlIx++] = subAttributes.buf;

  if (sqlIx > 0)
    pgCommands(sqlV, sqlIx);

  return true;
}
