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
#include "kjson/kjRender.h"                                    // kjRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate

#include "orionld/troe/PgTableDefinitions.h"                   // PG_ATTRIBUTE_INSERT_START, PG_SUB_ATTRIBUTE_INSERT_START
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppendInit.h"                         // pgAppendInit
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgAttributesBuild.h"                    // pgAttributesBuild
#include "orionld/troe/pgCommands.h"                           // pgCommands

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


  const char* opMode = (orionldState.uriParamOptions.noOverwrite == true)? "Append" : "Replace";

  PgAppendBuffer attributesBuffer;
  PgAppendBuffer subAttributesBuffer;

  pgAppendInit(&attributesBuffer, 2*1024);     // 2k - enough only for smaller entities - will be reallocated if necessary
  pgAppendInit(&subAttributesBuffer, 2*1024);  // ditto

  pgAppend(&attributesBuffer,    PG_ATTRIBUTE_INSERT_START,     0);
  pgAppend(&subAttributesBuffer, PG_SUB_ATTRIBUTE_INSERT_START, 0);

  char*    entityId = orionldState.wildcard[0];

  pgAttributesBuild(&attributesBuffer, orionldState.requestTree, entityId, opMode, &subAttributesBuffer);

  char* sqlV[2];
  int   sqlIx = 0;

  if (attributesBuffer.values    > 0) sqlV[sqlIx++] = attributesBuffer.buf;
  if (subAttributesBuffer.values > 0) sqlV[sqlIx++] = subAttributesBuffer.buf;

  if (sqlIx > 0)
    pgCommands(sqlV, sqlIx);

  return true;
}
