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
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/pgCompoundSubPropertyPush.h"            // Own interface



// -----------------------------------------------------------------------------
//
// pgCompoundPropertyPush - push a Compound Sub-Property to its DB table
//
bool pgCompoundSubPropertyPush
(
  PGconn*      connectionP,
  const char*  subAttributeName,
  const char*  instanceId,
  KjNode*      compoundValueNodeP,
  const char*  entityId,
  const char*  attrInstanceId,
  const char*  observedAt
)
{
  int          renderedValueSize   = 4 * 1024;
  char*        renderedValue       = kaAlloc(&orionldState.kalloc, renderedValueSize);
  int          sqlSize             = 5 * 1024;
  char*        sql                 = kaAlloc(&orionldState.kalloc, sqlSize);  // FIXME - one single call to kaAlloc, por favor !!!
  PGresult*    res;

  if ((renderedValue == NULL) || (sql == NULL))
    LM_RE(false, ("Internal Error (unable to allocate room for compound value of sub-attribute"));

  kjFastRender(orionldState.kjsonP, compoundValueNodeP, renderedValue, renderedValueSize);

  //
  // Two combinations for NULL/non-NULL 'observedAt'
  //
  if (observedAt != NULL)
  {
    snprintf(sql, sqlSize, "INSERT INTO subAttributes("
             "instanceId, id, entityId, attrInstanceId, ts, observedAt, valueType, compound) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'Compound', '%s')",
             instanceId, subAttributeName, entityId, attrInstanceId, orionldState.requestTimeString, observedAt, renderedValue);
  }
  else
  {
    snprintf(sql, sqlSize, "INSERT INTO subAttributes("
             "instanceId, id, entityId, attrInstanceId, ts, valueType, compound) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'Compound', '%s')",
             instanceId, subAttributeName, entityId, attrInstanceId, orionldState.requestTimeString, renderedValue);
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
