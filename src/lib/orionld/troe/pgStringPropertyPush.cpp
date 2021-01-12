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
#include "orionld/troe/pgStringPropertyPush.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// pgStringPropertyPush - push a String Property to its DB table
//
bool pgStringPropertyPush
(
  PGconn*      connectionP,
  const char*  opMode,
  const char*  value,
  const char*  entityId,
  const char*  attributeName,
  const char*  attributeInstance,
  const char*  datasetId,
  const char*  observedAt,
  bool         subProperties
)
{
  char         sql[1024];
  PGresult*    res;
  const char*  subPropertiesString = (subProperties == false)? "false" : "true";

  //
  // Four combinations for NULL/non-NULL 'datasetId' and 'observedAt'
  //
  if ((datasetId != NULL) && (observedAt != NULL))
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityId, ts, observedAt, valueType, subProperties, datasetId, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'String', %s, '%s', '%s')",
             opMode, attributeInstance, attributeName, entityId, orionldState.requestTimeString, observedAt, subPropertiesString, datasetId, value);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityId, ts, valueType, subProperties, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'String', %s, '%s')",
             opMode, attributeInstance, attributeName, entityId, orionldState.requestTimeString, subPropertiesString, value);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityId, ts, valueType, subProperties, datasetId, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'String', %s, '%s', '%s')",
             opMode, attributeInstance, attributeName, entityId, orionldState.requestTimeString, subPropertiesString, datasetId, value);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityId, ts, observedAt, valueType, subProperties, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'String', %s, '%s')",
             opMode, attributeInstance, attributeName, entityId, orionldState.requestTimeString, observedAt, subPropertiesString, value);
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
