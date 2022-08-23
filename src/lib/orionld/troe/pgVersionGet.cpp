/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <stdio.h>                                             // snprintf
#include <string.h>                                            // strncpy

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/pqHeader.h"                           // Postgres header
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease



// -----------------------------------------------------------------------------
//
// pgVersionToString -
//
void pgVersionToString(int version, char* versionString, int versionStringSize)
{
  int major;
  int minor;
  int bugfix;

  major    = version / 10000;
  version -= major * 10000;
  minor    = version / 100;
  version -= minor * 100;
  bugfix   = version;

  snprintf(versionString, versionStringSize - 1, "%d.%d.%d", major, minor, bugfix);
}



// -----------------------------------------------------------------------------
//
// pgVersionGet -
//
bool pgVersionGet(char* versionString, int versionStringSize)
{
  PgConnection*  connectionP     = pgConnectionGet(NULL);

  if ((connectionP != NULL) && (connectionP->connectionP != NULL))
  {
    int pgServerVersion = PQserverVersion(connectionP->connectionP);
    pgConnectionRelease(connectionP);

    pgVersionToString(pgServerVersion, versionString, versionStringSize);
    return true;
  }

  strncpy(versionString, "UNKNOWN", versionStringSize - 1);
  return false;
}
