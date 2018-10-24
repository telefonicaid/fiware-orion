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
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/context/orionldContextList.h"                // orionldContextHead, orionldContextTail
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/context/orionldContextAppend.h"              // Own interface



// ----------------------------------------------------------------------------
//
// orionldContextAppend - FIXME: better name. orionldContextCreateFromTreeAndInsert?
//
OrionldContext* orionldContextAppend(const char* url, KjNode* tree, OrionldContextType contextType, char** detailsPP)
{
  OrionldContext* contextP = orionldContextCreateFromTree(tree, url, contextType, detailsPP);

  if (contextP == NULL)
    return NULL;

  orionldContextListInsert(contextP);

  //
  // Presenting the list  (TMP)
  //
  LM_TMP(("Current context LIST:"));
  for (OrionldContext* ctxP = orionldContextHead; ctxP != NULL; ctxP = ctxP->next)
    LM_TMP(("o %p: %s, tree at %p", ctxP, ctxP->url, ctxP->tree));
  LM_TMP(("-------------------------------------------------------------------------------------------------------"));
  LM_TMP((""));

  return contextP;
}
