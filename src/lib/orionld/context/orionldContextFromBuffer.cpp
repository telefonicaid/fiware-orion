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
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjParse.h"                                       // kjParse
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldContext.h"                        // OrionldContext, OrionldContextOrigin
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/context/orionldContextFromTree.h"              // orionldContextFromTree
#include "orionld/context/orionldContextFromBuffer.h"            // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextFromBuffer -
//
OrionldContext* orionldContextFromBuffer(char* url, OrionldContextOrigin origin, char* id, char* buffer)
{
  if ((buffer == NULL) || (*buffer == 0))
  {
    orionldError(OrionldBadRequestData, "@context without content", url, 400);
    return NULL;
  }

  KjNode* tree = kjParse(kjsonP, buffer);

  if (tree == NULL)
  {
    char buf[256];
    strncpy(buf, buffer, sizeof(buf) - 1);
    LM_E(("JSON Parse Error for @context '%s' (first bytes of json: %s)", url, buf));

    orionldError(OrionldBadRequestData, "JSON Parse Error in @context", url, 400);
    return NULL;
  }

  KjNode* contextNodeP = kjLookup(tree, "@context");
  if (contextNodeP == NULL)
  {
    orionldError(OrionldInvalidRequest, "Invalid context - @context field missing", url, 400);
    return NULL;
  }

  LM_T(LmtCoreContext, ("Parsed the context buffer into a KjNode tree and looked up the @context member"));
  OrionldContext* contextP = orionldContextFromTree(url, origin, id, contextNodeP);
  return contextP;
}
