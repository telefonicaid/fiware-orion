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
#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/types/OrionldHeader.h"                       // orionldHeaderAdd
#include "orionld/service/orionldServiceLookup.h"              // orionldServiceLookup
#include "orionld/service/orionldServiceInit.h"                // orionldRestServiceV
#include "orionld/serviceRoutines/orionldPatchEntity2.h"       // orionldPatchEntity2
#include "orionld/serviceRoutines/orionldPutEntity.h"          // orionldPutEntity
#include "orionld/serviceRoutines/orionldOptions.h"            // Own interface



// -----------------------------------------------------------------------------
//
// orionldAllowedVerbs - get a list of allowed verbs for a URL PATH
//
// FIXME: orionldBadVerb does the exact same thing. This functions should have its own module.
//
static char* orionldAllowedVerbs(char* verbList, int len, char* serviceUrlPath, bool* patchAllowedP, bool* mergePatchP)
{
  uint16_t  bitmask = 0;

  //
  // There are nine verbs/methods, but only GET, POST, PATCH and DELETE are supported by ORIONLD
  // This loop looks up the URL PATH for each "orionld-valid" verb and keeps a bitmask of the hits
  //
  for (uint16_t verbNo = 0; verbNo <= HTTP_OPTIONS; verbNo++)  // 0:GET, 1:PUT, 2:POST, 3:DELETE, 4:PATCH, 5:HEAD, 6:OPTIONS
  {
    if (verbNo == HTTP_HEAD) continue;

    OrionLdRestService* serviceP;

    if ((serviceP = orionldServiceLookup(&orionldRestServiceV[verbNo])) != NULL)
    {
      // If not the exact same URL Path - not a match
      if (strcmp(serviceP->url, serviceUrlPath) != 0)
        continue;

      // PATCH Entity2 is only active if broker is started with -experimental
      if ((serviceP->serviceRoutine == orionldPatchEntity2) && (experimental == false))
        continue;
      // PUT Entity is only active if broker is started with -experimental
      if ((serviceP->serviceRoutine == orionldPutEntity) && (experimental == false))
        continue;

      bitmask |= (1 << verbNo);
      if (verbNo == HTTP_PATCH)
      {
        *patchAllowedP = true;
        if (serviceP->serviceRoutine == orionldPatchEntity2)
          *mergePatchP = true;
      }
    }
  }

  verbList[0] = 0;

  if (bitmask > 0)
  {
    if (bitmask & (1 << HTTP_GET))      strcat(verbList, ",GET");
    if (bitmask & (1 << HTTP_PUT))      strcat(verbList, ",PUT");
    if (bitmask & (1 << HTTP_POST))     strcat(verbList, ",POST");
    if (bitmask & (1 << HTTP_DELETE))   strcat(verbList, ",DELETE");
    if (bitmask & (1 << HTTP_PATCH))    strcat(verbList, ",PATCH");
    if (bitmask & (1 << HTTP_OPTIONS))  strcat(verbList, ",OPTIONS");

    verbList = &verbList[1];  // Skipping first comma
  }

  return verbList;
}



// ----------------------------------------------------------------------------
//
// orionldOptions -
//
bool orionldOptions(void)
{
  char  verbListV[256];
  char* verbList;
  bool  patchAllowed = false;
  bool  mergePatch   = false;

  verbList = orionldAllowedVerbs(verbListV, sizeof(verbListV), orionldState.serviceP->url, &patchAllowed, &mergePatch);

  if (patchAllowed == true)
  {
    if (mergePatch == true)
      orionldHeaderAdd(&orionldState.out.headers, HttpAcceptPatch, "application/json, application/ld+json, application/merge-patch+json", 0);
    else
      orionldHeaderAdd(&orionldState.out.headers, HttpAcceptPatch, "application/json, application/ld+json", 0);
  }

  // Allow HTTP header
  orionldHeaderAdd(&orionldState.out.headers, HttpAllow, verbList, 0);

  orionldState.httpStatusCode = 200;
  return true;
}
