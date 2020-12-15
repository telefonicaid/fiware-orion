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

#include "orionld/temporal/pgAttributeDelete.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// pgAttributeDelete - mark an attribute as deleted in the database
//
bool pgAttributeDelete(PGconn* connectionP, char* entityId, char* instanceId, char* attributeName, char* deletedAt)
{
  char       sql[512];
  PGresult*  res;

  snprintf(sql, sizeof(sql), "INSERT INTO attributes(opMode, instanceId, id, entityId, createdAt, modifiedAt, deletedAt) VALUES ('Delete', '%s', '%s', '%s', '%s', '%s', '%s')",
           instanceId, attributeName, entityId, deletedAt, deletedAt, deletedAt);

  LM_TMP(("SQL[%p]: %s", connectionP, sql));
  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));

  return true;
}
