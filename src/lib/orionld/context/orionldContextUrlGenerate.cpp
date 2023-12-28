/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
}

#include "logMsg/logMsg.h"                                       // LM*
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/orionldState.h"                         // orionldHostName, orionldHostNameLen
#include "orionld/context/orionldContextUrlGenerate.h"           // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextUrlGenerate -
//
// The size used in the call to kaAlloc:
//   - strlen("http://HOSTNAME:PORT"):         13 (port is max 5 chars - 65535) + orionldHostNameLen
//   - strlen("/ngsi-ld/v1/jsonldContexts/"):  27
//   - uuidGenerate:                           37
//   - zero termination:                        1
//
//  => 78 + orionldHostNameLen
//
char* orionldContextUrlGenerate(char** contextIdP)
{
  int   urlSize   = 78 + orionldHostNameLen;
  char* url       = (char*) kaAlloc(&kalloc, urlSize);
  int   prefixLen = snprintf(url, urlSize, "http://%s:%d/ngsi-ld/v1/jsonldContexts/", orionldHostName, portNo);

  *contextIdP = uuidGenerate(&url[prefixLen], urlSize - prefixLen, NULL);

  return url;
}
