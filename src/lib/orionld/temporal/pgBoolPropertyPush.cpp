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

#include "orionld/temporal/pgStringPropertyPush.h"             // Own interface



// -----------------------------------------------------------------------------
//
// pgBoolPropertyPush - push a String Property to its DB table
//
bool pgBoolPropertyPush
(
  PGconn*      connectionP,
  const char*  opMode,
  bool         value,
  const char*  entityRef,
  const char*  entityId,
  const char*  attributeName,
  const char*  attributeInstance,
  const char*  datasetId,
  const char*  observedAt,
  const char*  createdAt,
  const char*  modifiedAt,
  bool         subProperties
)
{
  char         sql[1024];
  PGresult*    res;
  const char*  subPropertiesString = (subProperties == false)? "false" : "true";
  const char*  boolValueAsString   = (value == true)? "true" : "false";

  //
  // Four combinations for NULL/non-NULL 'datasetId' and 'observedAt'
  //
  if ((datasetId != NULL) && (observedAt != NULL))
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, datasetId, boolean) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Boolean', %s, '%s', %s)",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, datasetId, boolValueAsString);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, boolean) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Boolean', %s, %s)",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, boolValueAsString);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, datasetId, boolean) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Boolean', %s, '%s', %s)",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, datasetId, boolValueAsString);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, boolean) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Boolean', %s, %s)",
             opMode, attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, boolValueAsString);
  }


  LM_TMP(("SQL[%p]: %s", connectionP, sql));
  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));

  if (PQstatus(connectionP) != CONNECTION_OK)
    LM_E(("SQL[%p]: bad connection: %d", connectionP, PQstatus(connectionP)));  // FIXME: string! (last error?)

  return true;
}
