/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include <stdio.h>                                             // snprintf

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgEntityAppend.h"                       // pgEntityAppend



// ----------------------------------------------------------------------------
//
// pgEntityAppend -
//
// INSERT INTO entities(instanceId,
//                      ts,
//                      opMode,
//                      id,
//                      type) VALUES (), (), ...
//
void pgEntityAppend(PgAppendBuffer* entitiesBufferP, const char* opMode, const char* entityId, const char* entityType, const char* instanceId)
{
  char         buf[1024];
  const char*  comma = (entitiesBufferP->values != 0)? "," : "";

  snprintf(buf, sizeof(buf), "%s('%s', '%s', '%s', '%s', '%s')", comma, instanceId, orionldState.requestTimeString, opMode, entityId, entityType);

  pgAppend(entitiesBufferP, buf, 0);
  entitiesBufferP->values += 1;
}
