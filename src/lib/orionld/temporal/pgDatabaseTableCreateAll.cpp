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

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/temporal/pgTransactionBegin.h"               // pgTransactionBegin
#include "orionld/temporal/pgTransactionRollback.h"            // pgTransactionRollback
#include "orionld/temporal/pgTransactionCommit.h"              // pgTransactionCommit
#include "orionld/temporal/pgDatabaseTableCreateAll.h"         // Own interface



// -----------------------------------------------------------------------------
//
// pgDatabaseTableCreateAll -
//
bool pgDatabaseTableCreateAll(PGconn* connectionP)
{
  const char* entitiesSql = "CREATE TABLE IF NOT EXISTS entities ("
    "instanceId   TEXT PRIMARY KEY,"
    "id           TEXT NOT NULL,"
    "type         TEXT NOT NULL,"
    "createdAt    TIMESTAMP NOT NULL,"
    "modifiedAt   TIMESTAMP NOT NULL,"
    "deletedAt    TIMESTAMP)";

  const char* attributesSql = "CREATE TABLE IF NOT EXISTS attributes ("
    "instanceId      TEXT PRIMARY KEY,"
    "id              TEXT NOT NULL,"
    "entityRef       TEXT NOT NULL REFERENCES entities(instanceId),"
    "entityId        TEXT NOT NULL,"
    "createdAt       TIMESTAMP NOT NULL,"
    "modifiedAt      TIMESTAMP NOT NULL,"
    "deletedAt       TIMESTAMP,"
    "observedAt      TIMESTAMP,"
    "valueType       ValueType,"
    "subProperty     BOOL,"
    "unitCode        TEXT,"
    "datasetId       TEXT,"
    "text            TEXT,"
    "boolean         BOOL,"
    "number          FLOAT8,"
    "datetime        TIMESTAMP,"
    "geo             GEOMETRY)";

  const char* subAttributesSql = "CREATE TABLE IF NOT EXISTS subAttributes ("
    "instanceId      TEXT PRIMARY KEY,"
    "id              TEXT NOT NULL,"
    "entityId        TEXT NOT NULL,"
    "entityRef       TEXT NOT NULL REFERENCES entities(instanceId),"
    "attributeId     TEXT NOT NULL,"
    "attributeRef    TEXT NOT NULL REFERENCES attributes(instanceId),"
    "createdAt       TIMESTAMP NOT NULL,"
    "modifiedAt      TIMESTAMP NOT NULL,"
    "deletedAt       TIMESTAMP,"
    "observedAt      TIMESTAMP,"
    "valueType       ValueType,"
    "text            TEXT,"
    "boolean         BOOL,"
    "number          FLOAT8,"
    "datetime        TIMESTAMP,"
    "geo             GEOMETRY,"
    "unitCode        TEXT)";

  const char* valueTypeSql = "CREATE TYPE ValueType AS ENUM("
    "'String',"
    "'Number',"
    "'Boolean',"
    "'Relationship',"
    "'Compound',"
    "'DateTime',"
    "'Geo',"
    "'LanguageMap')";

  const char* sqlV[] = { valueTypeSql, entitiesSql, attributesSql, subAttributesSql };

  pgTransactionBegin(connectionP);

  for (unsigned int ix = 0; ix < sizeof(sqlV) / sizeof(sqlV[0]); ix++)
  {
    PGresult*  res = PQexec(connectionP, sqlV[ix]);
    if (res == NULL)
    {
      pgTransactionRollback(connectionP);
      LM_RE(false, ("Database Error (PQexec(%s): %s)", sqlV[ix], PQresStatus(PQresultStatus(res))));
    }
  }

  pgTransactionCommit(connectionP);
  return true;
}
