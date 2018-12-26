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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjParse.h"                                     // kjParse
#include "kjson/kjFree.h"                                      // kjFree
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextDownloadAndParse.h"    // orionldContextDownloadAndParse
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextCreateFromUrl.h"       // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextCreateFromUrl -
//
OrionldContext* orionldContextCreateFromUrl(ConnectionInfo* ciP, const char* url, OrionldContextType contextType, char** detailsPP)
{
  OrionldContext* contextP = orionldContextLookup(url);

  //
  // If found, use it
  //
  if (contextP != NULL)
  {
    LM_TMP(("Found context '%s' - no need for creation", url));
    return contextP;
  }

  //
  // Else, create a new tree
  //
  LM_TMP(("Did not find context '%s', creating it", url));
  contextP = orionldContextCreateFromTree(NULL, url, contextType, detailsPP);

  if (contextP == NULL)
  {
    LM_E(("orionldContextCreateFromTree: %s", *detailsPP));
    ciP->contextToBeFreed = false;
    return NULL;
  }

  LM_TMP(("Calling orionldContextDownloadAndParse"));
  contextP->tree = orionldContextDownloadAndParse(ciP->kjsonP, url, detailsPP);
  LM_TMP(("orionldContextDownloadAndParse returned tree at %p", contextP->tree));
  if (contextP->tree == NULL)
  {
    LM_E(("orionldContextDownloadAndParse: %s", *detailsPP));
    free(contextP->url);
    free(contextP);
    ciP->contextToBeFreed = false;
    return NULL;
  }

  ciP->contextToBeFreed = true;
  LM_TMP(("Inserting context '%s' in common list", url));
  orionldContextListInsert(contextP);
  LM_TMP(("Inserted context '%s' in common list", url));

  return contextP;
}
