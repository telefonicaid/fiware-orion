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

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/contextCache/orionldContextCacheLookup.h"      // orionldContextCacheLookup
#include "orionld/serviceRoutines/orionldGetContext.h"           // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetContext -
//
bool orionldGetContext(ConnectionInfo* ciP)
{
  OrionldContext* contextP    = orionldContextCacheLookup(orionldState.wildcard[0]);

  orionldState.noLinkHeader = true;  // We don't want the Link header for context requests

  if (contextP == NULL)
  {
    orionldErrorResponseCreate(OrionldResourceNotFound, "Context Not Found", orionldState.wildcard[0]);
    orionldState.httpStatusCode = 404;
    return false;
  }

  orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);
  if (orionldState.responseTree == NULL)
  {
    LM_E(("Internal Error (out of memory)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "kjObject failed", "out of memory?");
    orionldState.httpStatusCode = SccReceiverInternalError;
    return false;
  }

  contextP->tree->name = (char*) "@context";

  kjChildAdd(orionldState.responseTree, contextP->tree);

  return true;
}
