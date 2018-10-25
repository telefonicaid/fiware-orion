/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string.h>                                            // strdup
#include <stdlib.h>                                            // malloc

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjParse.h"                                     // kjParse
#include "kjson/kjFree.h"                                      // kjFree
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/context/orionldCoreContext.h"                // ORIONLD_CORE_CONTEXT_URL, ORIONLD_DEFAULT_URL_CONTEXT_URL
#include "orionld/context/orionldContextCreateFromTree.h"      // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextCreateFromTree -
//
OrionldContext* orionldContextCreateFromTree(KjNode* tree, const char* url, OrionldContextType contextType, char** detailsPP)
{
  OrionldContext* contextP = (OrionldContext*) malloc(sizeof(OrionldContext));

  LM_T(LmtContext, ("Creating Context '%s'", url));

  if (contextP == NULL)
  {
    *detailsPP = (char*) "out of memory";
    return NULL;
  }

  contextP->url     = strdup(url);
  contextP->tree    = tree;
  contextP->type    = contextType;
  contextP->ignore  = false;
  contextP->next    = NULL;

  //
  // Lookups in USER contexts must not include the Core Context nor the Default URL Context
  // If a user context include any of these two, that part of the total context must be ignored.
  // Nothing is removed from the total context, so that the context can be retreived intact, but the
  // Core part must not be used in the lookups.
  //
  // FIXME TPUT: A char-sum would make these comparisons faster 
  //
  if ((strcmp(contextP->url, ORIONLD_CORE_CONTEXT_URL) == 0) || (strcmp(contextP->url, ORIONLD_DEFAULT_URL_CONTEXT_URL) == 0))
  {
    LM_T(LmtContext, ("Context '%s' is IGNORED", contextP->url));
    contextP->ignore = true;
  }
  else
    LM_T(LmtContext, ("Context '%s' is NOT ignored", contextP->url));
    
  return contextP;
}
