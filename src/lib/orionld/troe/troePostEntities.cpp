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
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"            // orionldAttributeExpand
#include "orionld/context/orionldSubAttributeExpand.h"         // orionldSubAttributeExpand
#include "orionld/troe/PgTableDefinitions.h"                   // PG_ATTRIBUTE_INSERT_START, PG_SUB_ATTRIBUTE_INSERT_START
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppendInit.h"                         // pgAppendInit
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgEntityBuild.h"                        // pgEntityBuild
#include "orionld/troe/pgCommands.h"                           // pgCommands
#include "orionld/troe/troePostEntities.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// troeSubAttrsExpand -
//
static void troeSubAttrsExpand(KjNode* attrP)
{
  KjNode* subAttrP = attrP->value.firstChildP;
  KjNode* nextSubAttr;

  while (subAttrP != NULL)
  {
    nextSubAttr = subAttrP->next;

    if      (strcmp(subAttrP->name,  "type")        == 0) {}
    else if (strcmp(subAttrP->name,  "id")          == 0) {}
    else if (strcmp(subAttrP->name,  "value")       == 0) {}
    else if (strcmp(subAttrP->name,  "object")      == 0) {}
    else if ((strcmp(subAttrP->name, "createdAt")   == 0) || (strcmp(subAttrP->name, "modifiedAt") == 0))
      kjChildRemove(attrP, subAttrP);
    else
      subAttrP->name = orionldSubAttributeExpand(orionldState.contextP, subAttrP->name,  true, NULL);

    subAttrP = nextSubAttr;
  }
}



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

    if      (strcmp(attrP->name, "type")              == 0)
      attrP->value.s = orionldContextItemExpand(orionldState.contextP, attrP->value.s, true, NULL);  // entity type
    else if (strcmp(attrP->name,  "id")               == 0) {}
    else if (strcmp(attrP->name,  "location")         == 0) {}
    else if (strcmp(attrP->name,  "observationSpace") == 0) {}
    else if (strcmp(attrP->name,  "operationSpace")   == 0) {}
    else if ((strcmp(attrP->name, "createdAt")        == 0) || (strcmp(attrP->name, "modifiedAt") == 0))
    {
      kjChildRemove(entityP, attrP);
    }
    else
    {
      attrP->name = orionldAttributeExpand(orionldState.contextP, attrP->name, true, NULL);

      if (attrP->type == KjObject)
        troeSubAttrsExpand(attrP);
      else if (attrP->type == KjArray)
      {
        for (KjNode* attrInstanceP = attrP->value.firstChildP; attrInstanceP != NULL; attrInstanceP = attrInstanceP->next)
        {
          troeSubAttrsExpand(attrInstanceP);
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
  char*    entityId    = (orionldState.payloadIdNode   != NULL)? orionldState.payloadIdNode->value.s   : NULL;
  char*    entityType  = (orionldState.payloadTypeNode != NULL)? orionldState.payloadTypeNode->value.s : NULL;
  KjNode*  entityP     = orionldState.requestTree;

  // Expand entity type and attribute names - FIXME: Remove once orionldPostEntities() has been fixed to do that
  troeEntityExpand(entityP);

  if (entityType != NULL)
    entityType = orionldContextItemExpand(orionldState.contextP, entityType, true, NULL);

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
