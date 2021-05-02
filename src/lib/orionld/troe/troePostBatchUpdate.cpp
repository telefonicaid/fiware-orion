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
#include "kbase/kMacros.h"                                     // K_FT
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/troeIgnored.h"                        // troeIgnored

#include "orionld/troe/PgTableDefinitions.h"                   // PG_ATTRIBUTE_INSERT_START, PG_SUB_ATTRIBUTE_INSERT_START
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppendInit.h"                         // pgAppendInit
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgAttributesBuild.h"                    // pgAttributesBuild
#include "orionld/troe/pgCommands.h"                           // pgCommands
#include "orionld/troe/troeEntityArrayExpand.h"                // troeEntityArrayExpand
#include "orionld/troe/troePostEntities.h"                     // Own interface



// ----------------------------------------------------------------------------
//
// troePostBatchUpdate -
//
bool troePostBatchUpdate(ConnectionInfo* ciP)
{
  PgAppendBuffer  attributes;
  PgAppendBuffer  subAttributes;
  const char*     attributeTroeMode = (orionldState.uriParamOptions.noOverwrite == true)? "Append" : "Replace";

  pgAppendInit(&attributes, 8*1024);     // 8k - will be reallocated if necessary
  pgAppendInit(&subAttributes, 8*1024);  // ditto

  pgAppend(&attributes,    PG_ATTRIBUTE_INSERT_START,     0);
  pgAppend(&subAttributes, PG_SUB_ATTRIBUTE_INSERT_START, 0);

  if (orionldState.duplicateArray != NULL)
  {
    troeEntityArrayExpand(orionldState.duplicateArray);
    for (KjNode* entityP = orionldState.duplicateArray->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      pgAttributesBuild(&attributes, entityP, NULL, attributeTroeMode, &subAttributes);
    }
  }

  troeEntityArrayExpand(orionldState.requestTree);
  for (KjNode* entityP = orionldState.requestTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    if (troeIgnored(entityP) == true)
      continue;

    pgAttributesBuild(&attributes, entityP, NULL, attributeTroeMode, &subAttributes);
  }

  const char* sqlV[2]  = { attributes.buf, subAttributes.buf };
  int         commands = 1;

  if (subAttributes.values > 0)
    ++commands;

  pgCommands(sqlV, commands);

  return true;
}
