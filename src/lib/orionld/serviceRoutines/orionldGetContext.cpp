/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/contextCache/orionldContextCacheLookup.h"      // orionldContextCacheLookup
#include "orionld/serviceRoutines/orionldGetContext.h"           // Own Interface



extern KjNode* orionldContextWithDetails(OrionldContext* contextP);
// ----------------------------------------------------------------------------
//
// orionldGetContext -
//
bool orionldGetContext(void)
{
  OrionldContext* contextP    = orionldContextCacheLookup(orionldState.wildcard[0]);

  orionldState.noLinkHeader = true;  // We don't want the Link header for context requests

  if (contextP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Context Not Found", orionldState.wildcard[0], 404);
    return false;
  }

  //
  // Contexts of type Cached (indirect download) are not served.
  // The broker is not a context server.
  // See 5.13.4.4 of the NGSI-LD API Specification
  //
  if ((contextP->kind == OrionldContextCached) && (orionldState.uriParams.details == false))
  {
    orionldError(OrionldOperationNotSupported, "Not serving cached JSON-LD @context", orionldState.wildcard[0], 422);
    return false;
  }

  if (orionldState.uriParams.details == true)
    orionldState.responseTree = orionldContextWithDetails(contextP);
  else
  {
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);
    if (orionldState.responseTree == NULL)
    {
      orionldError(OrionldBadRequestData, "kjObject failed", "out of memory?", 500);
      return false;
    }

    contextP->tree->name = (char*) "@context";
    kjChildAdd(orionldState.responseTree, contextP->tree);
  }

  return true;
}
