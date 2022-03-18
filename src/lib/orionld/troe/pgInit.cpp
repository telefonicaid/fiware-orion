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

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/pqHeader.h"                           // Postgres header
#include "orionld/common/orionldState.h"                       // troePort
#include "orionld/troe/pgConnectionPoolInit.h"                 // pgConnectionPoolInit
#include "orionld/troe/pgDatabasePrepare.h"                    // pgDatabasePrepare
#include "orionld/troe/pgInit.h"                               // Own interface


extern void pgConnectionPoolsPresent(void);
// -----------------------------------------------------------------------------
//
// pgInit -
//
bool pgInit(const char* dbPrefix)
{
  snprintf(pgPortString, sizeof(pgPortString), "%d", troePort);

  if (pgConnectionPoolInit(10) == false)
    LM_RE(false, ("error initializing the postgres connection pools"));

  bool b = pgDatabasePrepare(dbPrefix);
  pgConnectionPoolsPresent();
  return b;
}
