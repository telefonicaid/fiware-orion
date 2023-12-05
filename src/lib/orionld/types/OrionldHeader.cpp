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
#include <stdio.h>                                   // snprintf
#include <unistd.h>                                  // NULL

extern "C"
{
#include "kalloc/kaAlloc.h"                          // kaAlloc
#include "kalloc/kaStrdup.h"                         // kaStrdup
}

#include "logMsg/logMsg.h"                           // LM_*

#include "orionld/common/orionldState.h"             // orionldState
#include "orionld/types/OrionldHeader.h"             // Own interface



// -----------------------------------------------------------------------------
//
// orionldHeaderName -
//
const char* orionldHeaderName[] = {
  "Content-Length",
  "Content-Type",
  "Accept",
  "Link",
  "Host",
  "Allow",
  "Expect",
  "Origin",
  "Connection",
  "User-Agent",
  "NGSILD-Tenant",
  "NGSILD-Scope",
  "NGSILD-Results-Count",
  "Location",
  "Fiware-Service",
  "Fiware-Servicepath",
  "X-Auth-Token",
  "X-Forwarded-For",
  "X-Real-IP",
  "Ngsiv2-AttrsFormat",
  "Fiware-Correlator",
  "Fiware-Total-Count",
  "Access-Control-Allow-Origin",
  "Access-Control-Allow-Headers",
  "Access-Control-Allow-Methods",
  "Access-Control-Max-Age",
  "Access-Control-Expose-Headers",
  "Accept-Patch",
  "Performance",
  "NGSILD-EntityMap"
};



// -----------------------------------------------------------------------------
//
// orionldHeaderSetCreate -
//
OrionldHeaderSet* orionldHeaderSetCreate(int headers)
{
  OrionldHeaderSet* setP = (OrionldHeaderSet*) kaAlloc(&orionldState.kalloc, sizeof(OrionldHeaderSet));

  if (setP == NULL)
    LM_RE(NULL, ("Error allocating a OrionldHeaderSet of %d bytes", sizeof(OrionldHeaderSet)));

  if (orionldHeaderSetInit(setP, headers) == true)
    return setP;

  return NULL;
}



// -----------------------------------------------------------------------------
//
// orionldHeaderSetInit -
//
bool orionldHeaderSetInit(OrionldHeaderSet* setP, int headers)
{
  setP->headerV  = (OrionldHeader*) kaAlloc(&orionldState.kalloc, sizeof(OrionldHeader) * headers);

  if (setP->headerV == NULL)
    LM_RE(false, ("Error allocating %d OrionldHeader items of %d bytes each", headers, sizeof(OrionldHeader)));

  setP->size = headers;
  setP->ix   = 0;

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldHeaderAdd -
//
int orionldHeaderAdd(OrionldHeaderSet* setP, OrionldHeaderType type, const char* sValue, int iValue)
{
  if (setP->ix >= setP->size)  // Reallocation needed (add 3)
  {
    OrionldHeader* headerV = (OrionldHeader*) kaAlloc(&orionldState.kalloc, sizeof(OrionldHeader) * (setP->size + 3));

    if (headerV == NULL)
      LM_RE(-1, ("Error allocating %d OrionldHeader items of %d bytes each", setP->size + 3, sizeof(OrionldHeader)));

    // Copy old content to new array
    memcpy(headerV, setP->headerV, sizeof(OrionldHeader) * setP->size);

    // Point to the new array
    setP->headerV = headerV;

    // Increase size by 3
    setP->size += 3;
  }

  // Populate the last free slot
  OrionldHeader* headerP = &setP->headerV[setP->ix];

  headerP->type   = type;

  if (sValue == NULL)
  {
    headerP->iValue = iValue;

    headerP->sValue = kaAlloc(&orionldState.kalloc, 30);

    if (headerP->sValue == NULL)
      LM_RE(-1, ("Error allocating 30 bytes for a string buffer of an integer HTTP Header value"));
    snprintf(headerP->sValue, 30, "%d", headerP->iValue);
  }
  else
    headerP->sValue = kaStrdup(&orionldState.kalloc, sValue);

  setP->ix += 1;

  return 0;
}
