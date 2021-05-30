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
#include <stdlib.h>                                            // malloc, free
#include <string.h>                                            // strdup
#include <semaphore.h>                                         // sem_init

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/troe/PgConnectionPool.h"                     // PgConnectionPool
#include "orionld/troe/pgConnectionPoolCreate.h"               // Own interface



// -----------------------------------------------------------------------------
//
// pgConnectionPoolCreate -
//
PgConnectionPool* pgConnectionPoolCreate(const char* db, int poolSize)
{
  PgConnectionPool* poolP = (PgConnectionPool*) malloc(sizeof(PgConnectionPool));

  if (poolP == NULL)
    LM_RE(NULL, ("Out of memory (unable to allocate room for a postgres connection pool)"));

  poolP->connectionV = (PgConnection**) calloc(poolSize, sizeof(PgConnection*));
  if (poolP->connectionV == NULL)
  {
    free(poolP);
    LM_RE(NULL, ("Out of memory (unable to allocate room for the connections of a postgres connection pool)"));
  }

  if (db != NULL)
  {
    poolP->db = strdup(db);
    if (poolP->db == NULL)
    {
      free(poolP->connectionV);
      free(poolP);
      LM_RE(NULL, ("Out of memory (unable to allocate room for the DB-name of a postgres connection pool)"));
    }
  }
  else
    poolP->db = NULL;

  sem_init(&poolP->queueSem, 0, poolSize);  // Counting semaphore - there's a free slot
  sem_init(&poolP->poolSem,  0, 1);         // Binary semaphore - the pool is yours to modify

  poolP->items = poolSize;

  return poolP;
}
