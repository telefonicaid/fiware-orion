/*
*
* Copyright 2024 FIWARE Foundation e.V.
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
#include <stdio.h>                                               // snprintf

#include "logMsg/logMsg.h"

#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/http/httpHeaderLocationAdd.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// httpHeaderLocationAdd -
//
void httpHeaderLocationAdd(const char* uriPathWithSlash, const char* entityId, const char* tenant)
{
  char location[512];

  if (entityId != NULL)
    snprintf(location, sizeof(location), "%s%s", uriPathWithSlash, entityId);
  else
    snprintf(location, sizeof(location), "%s", uriPathWithSlash);

  orionldHeaderAdd(&orionldState.out.headers, HttpLocation, location, 0);

  if (tenant != NULL)
    orionldHeaderAdd(&orionldState.out.headers, HttpTenant, tenant, 0);
}
