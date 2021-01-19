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
#include <stdio.h>                                             // snprintf
#include <postgresql/libpq-fe.h>                               // PGconn

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/pgStringSubPropertyPush.h"              // Own interface



// -----------------------------------------------------------------------------
//
// pgStringSubPropertyPush - push a String Sub-Attribute to its DB table
//
bool pgStringSubPropertyPush
(
  PGconn*      connectionP,
  const char*  instanceId,
  const char*  stringValue,
  const char*  entityId,
  const char*  attributeId,
  const char*  subAttributeName,
  const char*  observedAt
)
{
  char         sql[2048];
  PGresult*    res;

  //
  // Two combinations for NULL/non-NULL 'observedAt'
  //
  if (observedAt != NULL)
  {
    snprintf(sql, sizeof(sql), "INSERT INTO subAttributes("
             "instanceId, id, entityId, attributeId, ts, observedAt, valueType, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'String', '%s')",
             instanceId, subAttributeName, entityId, attributeId, orionldState.requestTimeString, observedAt, stringValue);
  }
  else
  {
    snprintf(sql, sizeof(sql), "INSERT INTO subAttributes("
             "instanceId, id, entityId, attributeId, ts, valueType, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'String', '%s')",
             instanceId, subAttributeName, entityId, attributeId, orionldState.requestTimeString, stringValue);
  }

  LM_TMP(("SQL[%p]: %s;", connectionP, sql));
  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));
  PQclear(res);

  if (PQstatus(connectionP) != CONNECTION_OK)
    LM_E(("SQL[%p]: bad connection: %d", connectionP, PQstatus(connectionP)));  // FIXME: string! (last error?)

  return true;
}
