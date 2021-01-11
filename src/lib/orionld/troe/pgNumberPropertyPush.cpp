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
#include "orionld/troe/pgNumberPropertyPush.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// pgNumberPropertyPush - push a Number Property to its DB table
//
bool pgNumberPropertyPush
(
  PGconn*      connectionP,
  const char*  opMode,
  double       numberValue,
  const char*  entityRef,
  const char*  entityId,
  const char*  attributeName,
  const char*  attributeInstance,
  const char*  datasetId,
  const char*  observedAt,
  bool         subProperties,
  const char*  unitCode
)
{
  char         sql[1024];
  PGresult*    res;
  const char*  subPropertiesString = (subProperties == false)? "false" : "true";
  char         unitCodeStringV[128];
  char*        unitCodeString;

  if (unitCode == NULL)
    unitCodeString = (char*) "NULL";
  else
  {
    snprintf(unitCodeStringV, sizeof(unitCodeStringV), "'%s'", unitCode);
    unitCodeString = unitCodeStringV;
  }

  //
  // Four combinations for NULL/non-NULL 'datasetId' and 'observedAt'
  //
  if ((datasetId != NULL) && (observedAt != NULL))
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, ts, observedAt, valueType, subProperty, datasetId, number, unitCode) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Number', %s, '%s', %f, %s)",
             opMode, attributeInstance, attributeName, entityRef, entityId, orionldState.requestTimeString, observedAt, subPropertiesString, datasetId, numberValue, unitCodeString);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, ts, valueType, subProperty, number, unitCode) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'Number', %s, %f, %s)",
             opMode, attributeInstance, attributeName, entityRef, entityId, orionldState.requestTimeString, subPropertiesString, numberValue, unitCodeString);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, ts, valueType, subProperty, datasetId, number, unitCode) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'Number', %s, '%s', %f, %s)",
             opMode, attributeInstance, attributeName, entityRef, entityId, orionldState.requestTimeString, subPropertiesString, datasetId, numberValue, unitCodeString);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, sizeof(sql), "INSERT INTO attributes("
             "opMode, instanceId, id, entityRef, entityId, ts, observedAt, valueType, subProperty, number, unitCode) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Number', %s, %f, %s)",
             opMode, attributeInstance, attributeName, entityRef, entityId, orionldState.requestTimeString, observedAt, subPropertiesString, numberValue, unitCodeString);
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
