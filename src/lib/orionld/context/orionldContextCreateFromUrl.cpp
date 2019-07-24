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
#include "kjson/kjClone.h"                                     // kjClone
#include "kjson/kjFree.h"                                      // kjFree
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/context/orionldContextList.h"                // orionldContextListSemTake/Give
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
    LM_T(LmtContextList, ("Found context '%s' - no need for creation", url));
    return contextP;
  }

  //
  // Else, create a new tree
  // - First the context is allocated by calling orionldContextCreateFromTree (with tree set to NULL)
  // - Then the context is downloaded and parsed and the tree is created
  //
  LM_T(LmtContextList, ("Did not find context '%s', creating it", url));
  contextP = orionldContextCreateFromTree(NULL, url, contextType, detailsPP);

  if (contextP == NULL)
  {
    LM_E(("orionldContextCreateFromTree: %s", *detailsPP));
    orionldState.contextToBeFreed = false;
    ciP->httpStatusCode = SccBadRequest;
    return NULL;
  }

  contextP->tree = orionldContextDownloadAndParse(orionldState.kjsonP, url, true, detailsPP);
  if (contextP->tree == NULL)
  {
    LM_E(("orionldContextDownloadAndParse: %s", *detailsPP));
    free(contextP->url);
    free(contextP);
    orionldState.contextToBeFreed = false;
    ciP->httpStatusCode = SccBadRequest;
    return NULL;
  }

  if (contextP->tree && contextP->tree->value.firstChildP)
    LM_T(LmtContextList, ("The downloaded context (%s) is of type '%s'", contextP->url, kjValueType(contextP->tree->value.firstChildP->type)));
  else
    LM_E(("contextP->tree->value.firstChildP: %p", contextP->tree->value.firstChildP));

  orionldState.contextToBeFreed = false;
  LM_T(LmtContextList, ("Context is to be inserted into the common context list, so, it needs to be cloned"));

  // FIXME: Don't clone if core or vocab context
  contextP->tree = kjClone(contextP->tree);

  LM_T(LmtContextList, ("Inserting context '%s' in common list", url));

  orionldContextListInsert(contextP, false);

  if (contextP->tree->value.firstChildP->type == KjArray)
  {
    KjNode* arrayP = contextP->tree->value.firstChildP;

    LM_T(LmtContextList, ("We got an array - need to download more contexts"));
    for (KjNode* aItemP = arrayP->value.firstChildP; aItemP != NULL; aItemP = aItemP->next)
    {
      //
      // Is the context already present in the list?
      // If so, no need to download, parse and insert into the context list
      //
      if (aItemP->type != KjString)
        continue;

      if (orionldContextLookup(aItemP->value.s) != NULL)
        continue;

      if (urlCheck(aItemP->value.s, detailsPP) == false)
      {
        LM_E(("Context Array string-item is not a URL: %s", *detailsPP));
        ciP->httpStatusCode = SccBadRequest;
        return NULL;
      }

      OrionldContext*  arrayItemContextP;

      if ((arrayItemContextP = orionldContextCreateFromUrl(ciP, aItemP->value.s, OrionldUserContext, detailsPP)) == NULL)
      {
        LM_E(("orionldContextCreateFromUrl error: %s", *detailsPP));
        ciP->httpStatusCode = SccBadRequest;
        return NULL;
      }
      LM_T(LmtContextList, ("Inserting context '%s' in common list", url));
      orionldContextListInsert(arrayItemContextP, false);
    }
  }

  return contextP;
}
