/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <string.h>                                            // strerror
#include <errno.h>                                             // errno
#include <stddef.h>                                            // NULL
#include <semaphore.h>                                         // sem_t

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldContextList.h"                // Own interface



// ----------------------------------------------------------------------------
//
// orionldContextHead/Tail
//
OrionldContext* orionldContextHead = NULL;
OrionldContext* orionldContextTail = NULL;



// ----------------------------------------------------------------------------
//
// orionldContextListSem - semaphore for the context list
//
static sem_t orionldContextListSem;



// ----------------------------------------------------------------------------
//
// orionldContextListInit -
//
int orionldContextListInit(char** detailsP)
{
  orionldContextHead = NULL;
  orionldContextTail = NULL;

  if (sem_init(&orionldContextListSem, 0, 1) == -1)
  {
    LM_E(("Runtime Error (error initializing semaphore for orionld context list; %s)", strerror(errno)));
    *detailsP = (char*) "error initializing semaphore for orionld context list";
    return -1;
  }

  return 0;
}



// ----------------------------------------------------------------------------
//
// orionldContextListSemTake -
//
void orionldContextListSemTake(const char* who)
{
  LM_T(LmtContextList, ("%s taking semaphore", who));
  sem_wait(&orionldContextListSem);
  LM_T(LmtContextList, ("%s got semaphore", who));
}



// ----------------------------------------------------------------------------
//
// orionldContextListSemGive -
//
void orionldContextListSemGive(const char* who)
{
  LM_T(LmtContextList, ("%s giving semaphore", who));
  sem_post(&orionldContextListSem);
  LM_T(LmtContextList, ("%s gave semaphore", who));
}
