/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <time.h>
#include <semaphore.h>

#include "logMsg/logMsg.h"
#include "common/clockFunctions.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/MongoGlobal.h"



/* ****************************************************************************
*
* MongoConnection - 
*/
typedef struct MongoConnection
{
  DBClientBase*  connection;
  bool           free;
} MongoConnection;



/* ****************************************************************************
*
* globals - 
*/
static MongoConnection* connectionPool     = NULL;
static int              connectionPoolSize = 0;
static sem_t            connectionPoolSem;
static sem_t            connectionSem;
static struct timespec  semWaitingTime     = { 0, 0 };
static bool             semStatistics      = false;



/* ****************************************************************************
*
* mongoConnectionPoolInit - 
*/
int mongoConnectionPoolInit
(
  const char* host,
  const char* db,
  const char* rplSet,
  const char* username,
  const char* passwd,
  double      timeout,
  int         writeConcern,
  int         poolSize,
  bool        semTimeStat
)
{
  //
  // Create the pool
  //
  connectionPool     = (MongoConnection*) calloc(sizeof(MongoConnection), poolSize);
  if (connectionPool == NULL)
  {
    LM_E(("Runtime Error (insufficient memory to create connection pool of %d connections)", poolSize));
    return -1;
  }
  connectionPoolSize = poolSize;

  //
  // Initialize (connect) the pool
  //
  for (int ix = 0; ix < connectionPoolSize; ++ix)
  {
    connectionPool[ix].connection = mongoConnect(host, db, rplSet, username, passwd, writeConcern, timeout);
    connectionPool[ix].free       = true;
  }

  //
  // Set up the semaphore protecting the pool itself (connectionPoolSem)
  //
  int r = sem_init(&connectionPoolSem, 0, 1);

  if (r != 0)
  {
    LM_E(("Runtime Error (cannot create connection pool semaphore)"));
    return -1;
  }

  //
  // Set up the semaphore protecting the set of connections of the pool (connectionSem)
  // Note that this is a counting semaphore, initialized to connectionPoolSize.
  //
  r = sem_init(&connectionSem, 0, connectionPoolSize);
  if (r != 0)
  {
    LM_E(("Runtime Error (cannot create connection semaphore-set)"));
    return -1;
  }

  // Measure accumulated semaphore waiting time?
  semStatistics = semTimeStat;
  
  return 0;
}



/* ****************************************************************************
*
* mongoPoolConnectionGet - 
*
* There are two semaphores to get a connection. There is a limited number of connections
* and the first thing to do is to wait for a connection to become avilable (any of the
* connections in the pool) - this is done waiting on the counting semaphore that is 
* initialized with "POOL SIZE" - meaning the semaphore can be taken N times if the pool size is N.
* 
* Once a connection is free, 'sem_wait(&connectionSem)' returns and we now have to take the semaphore 
* that protects for pool itself (we *are* going to modify the vector of the pool - can only do it in
* one thread at a time ...)
* 
*/
DBClientBase* mongoPoolConnectionGet(void)
{
  DBClientBase*    connection = NULL;
  struct timespec  startTime;
  struct timespec  endTime;
  struct timespec  diffTime;

  if (semStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &startTime);
  }

  sem_wait(&connectionSem);
  sem_wait(&connectionPoolSem);

  if (semStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &endTime);

    clock_difftime(&endTime, &startTime, &diffTime);
    clock_addtime(&semWaitingTime, &diffTime);
  }

  for (int ix = 0; ix < connectionPoolSize; ++ix)
  {
    if (connectionPool[ix].free == true)
    {
      connectionPool[ix].free = false;
      connection = connectionPool[ix].connection;
      break;
    }
  }

  LM_M(("KZ: releasing connectionPoolSem"));
  sem_post(&connectionPoolSem);
  
  return connection;
}



/* ****************************************************************************
*
* mongoPoolConnectionRelease - 
*/
void mongoPoolConnectionRelease(DBClientBase* connection)
{
  LM_M(("KZ: Waiting for connectionPoolSem"));
  sem_wait(&connectionPoolSem);
  LM_M(("KZ: Got connectionPoolSem"));

  for (int ix = 0; ix < connectionPoolSize; ++ix)
  {
    if (connectionPool[ix].connection == connection)
    {
      connectionPool[ix].free = true;
      LM_M(("KZ: releasing connectionSem"));
      sem_post(&connectionSem);
      break;
    }
  }

  LM_M(("KZ: releasing connectionPoolSem"));
  sem_post(&connectionPoolSem);
}



/* ****************************************************************************
*
* mongoPoolConnectionSemWaitingTimeGet - 
*/
char* mongoPoolConnectionSemWaitingTimeGet(char* buf, int bufLen)
{
  if (semStatistics)
  {
    snprintf(buf, bufLen, "%lu.%09d", semWaitingTime.tv_sec, (int) semWaitingTime.tv_nsec);
  }
  else
  {
    snprintf(buf, bufLen, "Disabled");
  }

  return buf;
}



/* ****************************************************************************
*
* mongoPoolConnectionSemWaitingTimeReset - 
*/
void mongoPoolConnectionSemWaitingTimeReset(void)
{
  semWaitingTime.tv_sec  = 0;
  semWaitingTime.tv_nsec = 0;
}



