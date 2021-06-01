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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // troeHost, pgPortString, troeUser, troePwd
#include "orionld/troe/pgConnect.h"                            // pgConnect
#include "orionld/troe/PgConnection.h"                         // PgConnection
#include "orionld/troe/PgConnectionPool.h"                     // PgConnectionPool
#include "orionld/troe/pgConnectionPoolGet.h"                  // pgConnectionPoolGet
#include "orionld/troe/pgConnectionGet.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// wsTrim -
//
static char* wsTrim(char* s)
{
  // trim initial whitespace
  while ((*s == ' ') || (*s == '\t') || (*s == '\n'))
    ++s;

  // trim trailing whitespace
  int last = strlen(s);
  while ((s[last] == ' ') || (s[last] == '\t') || (s[last] == '\n'))
  {
    ++s;
  }
  s[last] = 0;

  return s;
}



// -----------------------------------------------------------------------------
//
// pgConnectionGet -
//
PgConnection* pgConnectionGet(const char* db)
{
  char* _db = (char*) db;

  //
  // Empty tenant => use default db name
  //
  // Note that a non-NULL 'db' that points to an empty string "" correcponds to the default database (orion)
  // Something very different is db == NULL - the "default" postgres database - to where we need to connect to create new databases
  //
  if ((db != NULL) && (*db == 0))
    _db = dbName;

  if (_db != NULL)
    _db = wsTrim(_db);

  // FIXME: Need a semaphore to protect the list of pools
  PgConnectionPool* poolP = pgConnectionPoolGet(_db);  // pgConnectionPoolGet creates the pool if it doesn't already exist

  if (poolP == NULL)
    LM_RE(NULL, ("unable to obtain a connection pool reference"));

  // Await a free slot in the pool
  sem_wait(&poolP->queueSem);

  // Await the right to modify the pool
  sem_wait(&poolP->poolSem);

  // Search for a free but already connected PgConnection in the pool
  for (int ix = 0; ix < poolP->items; ix++)
  {
    PgConnection* cP = poolP->connectionV[ix];

    if ((cP == NULL) || (cP->busy == true))
      continue;

    if (cP->connectionP != NULL)
    {
      // Great - found a free and already connected item - let's use it !
      cP->busy = true;

      sem_post(&poolP->poolSem);
      sem_post(&poolP->queueSem);

      cP->uses += 1;
      return cP;
    }
  }

  //
  // No already connected item was found - just look for an unused item and connect it to postgres
  //
  PgConnection* cP = NULL;
  for (int ix = 0; ix < poolP->items; ix++)
  {
    if (poolP->connectionV[ix] == NULL)
    {
      //
      // We found a completely unused slot - need to allocate
      // Pity doing this with the semaphore taken ...
      // But, there's no other choice as the slot must be marked as 'busy' before the sem can be released
      //
      poolP->connectionV[ix] = (PgConnection*) calloc(1, sizeof(PgConnection));
      if (poolP->connectionV[ix] == NULL)
      {
        sem_post(&poolP->poolSem);
        sem_post(&poolP->queueSem);
        LM_RE(NULL, ("Out of memory (unable to allocate room for a Postgres Connection - %d bytes)", sizeof(PgConnection)));
      }

      cP = poolP->connectionV[ix];
      break;
    }
    else if (poolP->connectionV[ix]->busy == false)
    {
      cP = poolP->connectionV[ix];
      break;
    }
  }


  if (cP != NULL)
    cP->busy = true;  // Now the pool item 'cP' is ours - after this we can let go of the semaphore

  sem_post(&poolP->poolSem);
  sem_post(&poolP->queueSem);

  if (cP == NULL)
    LM_RE(NULL, ("Internal Error (bug in postgres connection pool logic)"));

  if (cP->connectionP == NULL)  // Virgin connection
  {
    cP->connectionP = pgConnect(_db);
    if (cP->connectionP == NULL)
    {
      cP->busy = false;  // So the slot can be used again!
      LM_RE(NULL, ("Database Error (unable to connect to postgres(%s))", _db));
    }
  }

  cP->uses += 1;
  return cP;
}
