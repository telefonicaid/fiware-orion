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
#include <stdio.h>                                             // popen, fgets
#include <semaphore.h>                                         // sem_init, sem_take, ...
#include <postgresql/libpq-fe.h>                               // PGconn

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // troePwd, troeUser, troePoolSize
#include "orionld/troe/pgDatabasePrepare.h"                    // pgDatabasePrepare
#include "orionld/troe/pgInit.h"                               // Own interface



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



sem_t pgDbSem;

typedef struct PgConnection
{
  PGconn* connectionP;
  bool    taken;
} PgConnection;

typedef struct PgDb
{
  char          name[64];
  int           poolItems;
  PgConnection* pool;
} PgDb;

PgDb           pgDbV[100];
unsigned int   pgDbs = 0;



// -----------------------------------------------------------------------------
//
// pgDbInit -
//
void pgDbInit(void)
{
  sem_init(&pgDbSem, 0, 0);
  bzero(pgDbV, sizeof(pgDbV));
}



// -----------------------------------------------------------------------------
//
// pgDbRelease -
//
void pgDbRelease(void)
{
  for (unsigned int ix = 0; ix < pgDbs; ix++)
  {
    if (pgDbV[ix].pool != NULL)
    {
      for (int pIx = 0; pIx < pgDbV[ix].poolItems; pIx++)
      {
        if (pgDbV[ix].pool[pIx].connectionP != NULL)
          PQfinish(pgDbV[ix].pool[pIx].connectionP);
        pgDbV[ix].pool[pIx].connectionP = NULL;
      }

      free(pgDbV[ix].pool);
    }
  }
}



// -----------------------------------------------------------------------------
//
// pgDbAppend -
//
bool pgDbAppend(char* dbName)
{
  PgDb* dbP;

  if (pgDbs >= sizeof(pgDbV) / sizeof(pgDbV[0]))
    LM_RE(false, ("too many tenants used - change size of PG DB Array and recompile"));

  sem_wait(&pgDbSem);

  dbP = &pgDbV[pgDbs++];

  strncpy(dbP->name, dbName, sizeof(dbP->name));
  dbP->poolItems = 0;
  dbP->pool      = NULL;

  sem_post(&pgDbSem);

  return true;
}


// -----------------------------------------------------------------------------
//
// pgDbPoolSetup -
//
bool pgDbPoolSetup(PgDb* dbP)
{
  dbP->pool = (PgConnection*) calloc(troePoolSize, sizeof(PgConnection));

  if (dbP->pool == NULL)
    LM_RE(false, ("Internal Error (unable to allocate room for connection pool for postgres db '%s')", dbName));

  return true;
}


// -----------------------------------------------------------------------------
//
// pgDbPopulate -
//
bool pgDbPopulate(const char* dbPrefix)
{
  char   cmd[256];
  FILE*  pipe;

  snprintf(cmd, sizeof(cmd), "PGPASSWORD=%s psql -U %s --command '\\l'", troePwd, troeUser);
  if ((pipe = popen(cmd, "r")) == NULL)
    LM_RE(false, ("error getting databases via popen"));

  int lineNo = 0;
  while (fgets(cmd, sizeof(cmd), pipe) != NULL)
  {
    ++lineNo;
    if (lineNo < 3)  // First two lines are table title and separator
      continue;

    char* cP = strchr(cmd, '|');
    if (cP == NULL)
      continue;
    *cP = 0;

    char* dbNameP = cmd;

    wsTrim(dbNameP);

    if (strncmp(dbNameP, dbPrefix, strlen(dbPrefix)) != 0)
      continue;

    // FIXME: Check all tables are present and 100% OK
    pgDbAppend(dbNameP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pgInit -
//
bool pgInit(const char* dbPrefix)
{
#if 0
  pgDbInit();

  if (pgDbPopulate(dbPrefix) == false)
    LM_RE(false, ("pgDbPopulate failed"));
#endif
  return pgDatabasePrepare(dbPrefix);
}
