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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextAppend.h"              // Own interface



// ----------------------------------------------------------------------------
//
// orionldContextAppend - FIXME: better name. orionldContextCreateFromTreeAndInsert?
//
OrionldContext* orionldContextAppend(const char* url, KjNode* tree, OrionldContextType contextType, char** detailsPP)
{
  OrionldContext* contextP;

  // What if the context already exists ... ?
  if ((contextP = orionldContextLookup(url)) != NULL)
    return contextP;

  contextP = orionldContextCreateFromTree(tree, url, contextType, detailsPP);

  if (contextP == NULL)
  {
    LM_E(("Error creating context: %s", *detailsPP));
    return NULL;
  }

  orionldContextListInsert(contextP, false);

  return contextP;
}
