/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjRender.h"                                      // kjFasrRender
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/httpHeaderAdd.h"                                  // httpHeaderLocationAdd
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldContextUrlGenerate.h"           // orionldContextUrlGenerate
#include "orionld/context/orionldContextFromTree.h"              // orionldContextFromTree

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo



// ----------------------------------------------------------------------------
//
// orionldPostContexts -
//
// The @context can come in in various formats.
// 1. { "key1": "value1", "key2": "value2", ... }
// 2. [ "url1", "url2", ... ]
// 3. { "@context": { "key1": "value1", "key2": "value2", ... } }
// 4. { "@context": [ "url1", "url2", ... ] }
// 5. Etc
//
// orionldContextFromTree() needs the value of the @context as the input tree, so, in case the "@context" member is
// present, like in cases 3 and 4, then it will have to be extracted and its value used for orionldContextFromTree().
//
// orionldMhdConnectionTreat already finds and extracts the @context member if present, stores it in orionldState.payloadContextNode
//
bool orionldPostContexts(ConnectionInfo* ciP)
{
  char* id;
  char* url;

  if (orionldState.payloadContextNode != NULL)
    orionldState.requestTree = orionldState.payloadContextNode;

  url = orionldContextUrlGenerate(&id);

  OrionldProblemDetails  pd;
  OrionldContext*        contextP = orionldContextFromTree(url, OrionldContextUserCreated, id, true, orionldState.requestTree, &pd);

  if (contextP == NULL)
  {
    LM_W(("Unable to create context (%s: %s)", pd.title, pd.detail));
    orionldErrorResponseCreate(&pd);
    return false;
  }

  httpHeaderLocationAdd(ciP, contextP->url, NULL);
  orionldState.httpStatusCode = 201;

  return true;
}
