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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/http/httpHeaderLocationAdd.h"                  // httpHeaderLocationAdd
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/contextCache/orionldContextCacheGet.h"         // orionldContextCacheGet
#include "orionld/contextCache/orionldContextCacheLookup.h"      // orionldContextCacheLookup
#include "orionld/serviceRoutines/orionldGetContext.h"           // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetContexts -
//
bool orionldGetContexts(void)
{
  if (orionldState.uriParams.location == true)
  {
    if (orionldState.uriParams.url == NULL)
    {
      orionldError(OrionldBadRequestData, "Incompatible URI parameters", "'location' present but 'url' missing", 400);
      return false;
    }

    //
    // If both 'url' and 'location' URI params are present, then 'details' can't be there
    //
    if (orionldState.uriParams.details == true)
    {
      orionldError(OrionldBadRequestData, "Incompatible URI parameters", "'location' and 'url' don't support 'details'", 400);
      return false;
    }

    OrionldContext* contextP = orionldContextCacheLookup(orionldState.uriParams.url);

    if (contextP == NULL)
    {
      orionldError(OrionldResourceNotFound, "Context Not Found", orionldState.uriParams.url, 404);
      return false;
    }

    httpHeaderLocationAdd("/ngsi-ld/v1/jsonldContexts/", contextP->id, NULL);
    return true;
  }
  else if (orionldState.uriParams.url != NULL)
  {
    orionldError(OrionldBadRequestData, "Incompatible URI parameters", "'url' present but 'location' missing", 400);
    return false;
  }

  KjNode* contextTree = kjArray(orionldState.kjsonP, "contexts");

  orionldState.noLinkHeader = true;  // We don't want the Link header for context requests

  orionldState.responseTree = orionldContextCacheGet(contextTree, orionldState.uriParams.details);

  return true;
}
