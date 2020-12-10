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
#include "kjson/kjRender.h"                                    // kjRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/temporal/pgCompoundPropertyPush.h"           // Own interface



// -----------------------------------------------------------------------------
//
// pgCompoundPropertyPush - push a Compound Property to its DB table
//
// If the value is Compound - the need for unitCode seems ... ZERO - so, not supporting it !
//
bool pgCompoundPropertyPush
(
  PGconn*      connectionP,
  KjNode*      compoundValueNodeP,
  const char*  entityRef,
  const char*  entityId,
  const char*  attributeName,
  const char*  attributeInstance,
  const char*  datasetId,
  const char*  observedAt,
  const char*  createdAt,
  const char*  modifiedAt,
  bool         subProperties,
  const char*  unitCode
)
{
  int          renderedValueSize   = 4 * 1024;
  char*        renderedValue       = kaAlloc(&orionldState.kalloc, renderedValueSize);
  int          sqlSize             = 5 * 1024;
  char*        sql                 = kaAlloc(&orionldState.kalloc, sqlSize);  // FIXME - one single call to kaAlloc, por favor !!!
  const char*  subPropertiesString = (subProperties == false)? "false" : "true";
  PGresult*    res;

  if ((renderedValue == NULL) || (sql == NULL))
    LM_RE(false, ("Internal Error (unable to allocate room for compound value"));

  kjRender(orionldState.kjsonP, compoundValueNodeP, renderedValue, renderedValueSize);

  //
  // Four combinations for NULL/non-NULL 'datasetId' and 'observedAt'
  //
  if ((datasetId != NULL) && (observedAt != NULL))
  {
    snprintf(sql, sqlSize, "INSERT INTO attributes("
             "instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, datasetId, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Compound', %s, '%s', '%s')",
             attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, datasetId, renderedValue);
  }
  else if ((datasetId == NULL) && (observedAt == NULL))
  {
    snprintf(sql, sqlSize, "INSERT INTO attributes("
             "instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'Compound', %s, '%s')",
             attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, renderedValue);
  }
  else if (datasetId != NULL)  // observedAt == NULL
  {
    snprintf(sql, sqlSize, "INSERT INTO attributes("
             "instanceId, id, entityRef, entityId, createdAt, modifiedAt, valueType, subProperty, datasetId, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'Compound', %s, '%s', '%s')",
             attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, subPropertiesString, datasetId, renderedValue);
  }
  else  // observedAt != NULL, datasetId == NULL
  {
    snprintf(sql, sqlSize, "INSERT INTO attributes("
             "instanceId, id, entityRef, entityId, createdAt, modifiedAt, observedAt, valueType, subProperty, text) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', 'Compound', %s, '%s')",
             attributeInstance, attributeName, entityRef, entityId, createdAt, modifiedAt, observedAt, subPropertiesString, renderedValue);
  }

  // <DEBUG>
  char  c   = 0;
  char* cut = (char*) "(uncut)";

  if (strlen(sql) > 1024 * 2)
  {
    c = sql[1024 * 2];
    sql[1024 * 2] = 0;
    cut = (char*) "cut at 2K";
  }

  LM_TMP(("SQL[%p]: %s %s", connectionP, sql, cut));

  if (c != 0)
    sql[1024 * 2] = c;
  // </DEBUG>

  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));

  if (PQstatus(connectionP) != CONNECTION_OK)
    LM_E(("SQL[%p]: bad connection: %d", connectionP, PQstatus(connectionP)));  // FIXME: string! (last error?)
  else
    LM_TMP(("SQL: DB operation to insert a Compound Property seems to have worked"));

  return true;
}
