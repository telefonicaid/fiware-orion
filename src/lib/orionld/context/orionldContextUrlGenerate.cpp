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

#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/orionldState.h"                         // orionldHostName, orionldHostNameLen
#include "orionld/context/orionldContextUrlGenerate.h"           // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextUrlGenerate -
//
// The size used in the call to kaAlloc:
//   - strlen("http://HOSTNAME:PORT"):       12
//   - strlen("/ngsi-ld/ex/v1/contexts/"):   24
//   - uuidGenerate:                         37
//   - zero termination:                     1
//   - orionldHostNameLen
//
//  => 74 + orionldHostNameLen
//
char* orionldContextUrlGenerate(char** contextIdP)
{
  char* url = (char*) kaAlloc(&kalloc, 74 + orionldHostNameLen);

  snprintf(url, 74 + orionldHostNameLen, "http://%s:%d/ngsi-ld/ex/v1/contexts/", orionldHostName, portNo);
  uuidGenerate(&url[36 + orionldHostNameLen]);

  *contextIdP = &url[36 + orionldHostNameLen];

  return url;
}
