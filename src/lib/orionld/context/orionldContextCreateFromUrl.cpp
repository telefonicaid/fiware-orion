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

#include "orionld/common/OrionldConnection.h"                  // orionldState
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
    ciP->contextToBeFreed = false;
    ciP->httpStatusCode = SccBadRequest;
    return NULL;
  }

  contextP->tree = orionldContextDownloadAndParse(orionldState.kjsonP, url, detailsPP);
  if (contextP->tree == NULL)
  {
    LM_E(("orionldContextDownloadAndParse: %s", *detailsPP));
    free(contextP->url);
    free(contextP);
    ciP->contextToBeFreed = false;
    ciP->httpStatusCode = SccBadRequest;
    return NULL;
  }

  // <DEBUG>
  extern void contextArrayPresent(KjNode* tree, const char* what);
  contextArrayPresent(contextP->tree, "Just after orionldContextDownloadAndParse");
  // </DEBUG>

  if (contextP->tree && contextP->tree->value.firstChildP)
    LM_T(LmtContextList, ("The downloaded context (%s) is of type '%s'", contextP->url, kjValueType(contextP->tree->value.firstChildP->type)));
  else
    LM_E(("contextP->tree->value.firstChildP: %p", contextP->tree->value.firstChildP));

  ciP->contextToBeFreed = true;
  LM_T(LmtContextList, ("Context is to be inserted into the common context list, so, it needs to be cloned"));

  // <DEBUG>
  contextArrayPresent(contextP->tree, "Before cloning");
  // </DEBUG>

  // FIXME: Don't clone if core or vocab context
  LM_TMP(("Cloning context tree"));
  contextP->tree = kjClone(contextP->tree);

  // <DEBUG>
  contextArrayPresent(contextP->tree, "Just after cloning");
  // </DEBUG>


  LM_T(LmtContextList, ("Inserting context '%s' in common list", url));

  LM_TMP(("Calling orionldContextListInsert for '%s' (after cloning tree)", contextP->url));
  orionldContextListInsert(contextP, false);

  if (contextP->tree->value.firstChildP->type == KjArray)
  {
    KjNode* arrayP = contextP->tree->value.firstChildP;

    LM_T(LmtContextList, ("We got an array - need to download more contexts"));
    for (KjNode* aItemP = arrayP->value.firstChildP; aItemP != NULL; aItemP = aItemP->next)
    {
      // All items must be strings
      if (aItemP->type != KjString)
      {
        LM_E(("Context Array item not a string"));
        ciP->httpStatusCode = SccBadRequest;
        return NULL;
      }
      LM_T(LmtContextList, ("Array item (URL of context that needs to be downloaded): %s", aItemP->value.s));

      //
      // Is the context already present in the list?
      // If so, no need to download, parse and insert into the context list
      //
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
      LM_TMP(("Calling orionldContextListInsert for '%s' (after NOT cloning tree)", arrayItemContextP->url));
      orionldContextListInsert(arrayItemContextP, false);
    }
  }

  return contextP;
}
