/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* fermin at tid dot es
*
* Author: Fermin Galan
*/
#include <semaphore.h>
#include <errno.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"



/* ****************************************************************************
*
* Globals -
*/
static sem_t reqSem;
static sem_t mongoSem;

/* ****************************************************************************
*
* semInit -
*
*   parameter #1: 0 - the semaphore is to be shared between threads,
*   parameter #2: 1 - initially the semaphore is free
*
* RETURN VALUE (of sem_init)
*   0 on success,
*  -1 on failure
*
*/
int semInit(int shared, int takenInitially)
{
  if (sem_init(&reqSem, shared, takenInitially) == -1)
  {
    LM_E(("Runtime Error (error initializing 'req' semaphore: %s)", strerror(errno)));
    return 1;
  }

  if (sem_init(&mongoSem, shared, takenInitially) == -1)
  {
    LM_E(("Runtime Error (error initializing 'mongo' semaphore: %s)", strerror(errno)));
    return 2;
  }

  LM_T(LmtReqSem,   ("Initialized 'req' semaphore"));
  LM_T(LmtMongoSem, ("Initialized 'mongo' semaphore"));

  return 0;
}

/* ****************************************************************************
*
* reqSemTake -
*/
int reqSemTake(const char* who, const char* what)
{
  int r;

  LM_T(LmtReqSem, ("%s taking the 'req' semaphore for '%s'", who, what));
  r = sem_wait(&reqSem);
  LM_T(LmtReqSem, ("%s has the 'req' semaphore", who));

  return r;
}

/* ****************************************************************************
*
* mongoSemTake -
*/
int mongoSemTake(const char* who, const char* what)
{
  int r;

  LM_T(LmtMongoSem, ("%s taking the 'mongo' semaphore for '%s'", who, what));
  r = sem_wait(&mongoSem);
  LM_T(LmtMongoSem, ("%s has the 'mongo' semaphore", who));

  return r;
}

/* ****************************************************************************
*
* reqSemGive -
*/
int reqSemGive(const char* who, const char* what)
{
  if (what != NULL)
    LM_T(LmtReqSem, ("%s gives the 'req' semaphore for '%s'", who, what));
  else
    LM_T(LmtReqSem, ("%s gives the 'req' semaphore", who));

  return sem_post(&reqSem);
}

/* ****************************************************************************
*
* mongoSemGive -
*/
int mongoSemGive(const char* who, const char* what)
{
  if (what != NULL)
    LM_T(LmtMongoSem, ("%s gives the 'mongo' semaphore for '%s'", who, what));
  else
    LM_T(LmtMongoSem, ("%s gives the 'mongo' semaphore", who));

  return sem_post(&mongoSem);
}
