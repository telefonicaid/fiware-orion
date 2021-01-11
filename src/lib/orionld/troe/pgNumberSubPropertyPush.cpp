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

#include "orionld/troe/pgNumberSubPropertyPush.h"              // Own interface



// -----------------------------------------------------------------------------
//
// pgNumberSubPropertyPush - push a Numberean Sub-Property to its DB table
//
bool pgNumberSubPropertyPush
(
  PGconn*      connectionP,
  const char*  subAttributeName,
  const char*  instanceId,
  double       numberValue,
  const char*  entityRef,
  const char*  entityId,
  const char*  attributeRef,
  const char*  attributeId,
  const char*  observedAt,
  const char*  createdAt,
  const char*  modifiedAt,
  const char*  unitCode
)
{
  char   sql[1024];
  char   unitCodeStringV[128];
  char*  unitCodeString;

  if (unitCode == NULL)
    unitCodeString = (char*) "NULL";
  else
  {
    snprintf(unitCodeStringV, sizeof(unitCodeStringV), "'%s'", unitCode);
    unitCodeString = unitCodeStringV;
  }

  //
  // Two combinations for NULL/non-NULL 'observedAt' (sub-attributes have no datasetId)
  //
  if (observedAt != NULL)
  {
    snprintf(sql, sizeof(sql), "INSERT INTO subAttributes("
             "instanceId, id, entityRef, entityId, attributeRef, attributeId, createdAt, modifiedAt, observedAt, valueType, number, unitCode) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Number', %f, %s)",
             instanceId, subAttributeName, entityRef, entityId, attributeRef, attributeId, createdAt, modifiedAt, observedAt, numberValue, unitCodeString);
  }
  else
  {
    snprintf(sql, sizeof(sql), "INSERT INTO subAttributes("
             "instanceId, id, entityRef, entityId, attributeRef, attributeId, createdAt, modifiedAt, valueType, number, unitCode) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Number', %f, %s)",
             instanceId, subAttributeName, entityRef, entityId, attributeRef, attributeId, createdAt, modifiedAt, numberValue, unitCodeString);
  }
  LM_TMP(("SQL[%p]: %s;", connectionP, sql));


  PGresult* res;
  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));
  PQclear(res);

  if (PQstatus(connectionP) != CONNECTION_OK)
    LM_E(("SQL[%p]: bad connection: %d", connectionP, PQstatus(connectionP)));  // FIXME: string! (last error?)

  return true;
}
