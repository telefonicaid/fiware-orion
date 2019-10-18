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
#include "logMsg/logMsg.h"                                  // LM_*
#include "logMsg/traceLevels.h"                             // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                   // KjNode
#include "kjson/kjRender.h"                                 // kjRender
#include "kjson/kjBufferCreate.h"                           // kjBufferCreate
}

#include "orionld/context/OrionldContext.h"                 // OrionldContext
#include "orionld/context/orionldContextList.h"             // orionldContextHead
#include "orionld/context/orionldContextPresent.h"          // Own interface



// ----------------------------------------------------------------------------
//
// allCachedContextsPresent -
//
static void allCachedContextsPresent(void)
{
  OrionldContext* contextP = orionldContextHead;

  while (contextP != NULL)
  {
    orionldContextPresent(contextP);
    contextP = contextP->next;
  }
}



// ----------------------------------------------------------------------------
//
// stringContextPresent -
//
static void stringContextPresent(OrionldContext* contextP)
{
  LM_T(LmtContextPresent, ("Context STRING: %s (ignored: %s):", contextP->url, (contextP->ignore == true)? "YES" : "NO"));
  LM_T(LmtContextPresent, ("  %s", contextP->tree->value.s));
}



// ----------------------------------------------------------------------------
//
// arrayContextPresent -
//
static void arrayContextPresent(OrionldContext* contextP)
{
  KjNode* itemP;

  LM_T(LmtContextPresent, ("Context ARRAY: %s (ignored: %s):", contextP->url, (contextP->ignore == true)? "YES" : "NO"));

  for (itemP = contextP->tree->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    LM_T(LmtContextPresent, ("  %s", itemP->value.s));
  }
}



// ----------------------------------------------------------------------------
//
// objectContextPresent -
//
static Kjson* kjsonBuffer = NULL;
static void objectContextPresent(OrionldContext* contextP)
{
  if (contextP == NULL)
  {
    LM_T(LmtContextPresent, ("contextP == NULL"));
    return;
  }

  if (kjsonBuffer == NULL)
  {
    kjsonBuffer = kjBufferCreate(NULL, NULL);
    if (kjsonBuffer == NULL)
      LM_X(1, ("Out of memory"));

    kjsonBuffer->spacesPerIndent   = 0;
    kjsonBuffer->nlString          = (char*) "";
    kjsonBuffer->stringBeforeColon = (char*) "";
    kjsonBuffer->stringAfterColon  = (char*) "";
  }

  LM_T(LmtContextPresent, ("Context '%s' (ignored: %s):", contextP->url, (contextP->ignore == true)? "YES" : "NO"));

  char buf[1024];

  kjRender(kjsonBuffer, contextP->tree, buf, sizeof(buf));
  buf[120] = 0;
  LM_T(LmtContextPresent, ("Context Tree: %s", buf));

#if 1
  if (contextP->tree == NULL)
  {
    LM_T(LmtContextPresent, ("contextP->tree is NULL"));
  }
  else if (contextP->tree->value.firstChildP == NULL)
  {
    LM_T(LmtContextPresent, ("contextP->tree is of type '%s'", kjValueType(contextP->tree->type)));
    LM_T(LmtContextPresent, ("contextP->tree->value.firstChildP is NULL"));
  }
  else if (contextP->tree->value.firstChildP->value.firstChildP == NULL)
  {
    LM_T(LmtContextPresent, ("contextP->tree->value.firstChildP->value.firstChildP is NULL"));
  }
  else
  {
    if (contextP->tree->value.firstChildP->type == KjString)
    {
      LM_T(LmtContextPresent, ("    %s: %s", contextP->tree->value.firstChildP->name, contextP->tree->value.firstChildP->value.s));
    }
    else if (contextP->tree->value.firstChildP->type == KjObject)
    {
      for (KjNode* nodeP = contextP->tree->value.firstChildP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
      {
        if (nodeP->type == KjString)
        {
          LM_T(LmtContextPresent, ("  %s: %s", nodeP->name, nodeP->value.s));
        }
        else if (nodeP->type == KjObject)
        {
          LM_T(LmtContextPresent, ("  %s: {", nodeP->name));

          for (KjNode* childP = nodeP->value.firstChildP; childP != NULL; childP = childP->next)
            LM_T(LmtContextPresent, ("    %s: %s", childP->name, childP->value.s));
          LM_T(LmtContextPresent, ("  }"));
        }
      }
    }
  }
#endif
}


// ----------------------------------------------------------------------------
//
// orionldContextPresent -
//
void orionldContextPresent(OrionldContext* contextP)
{
  //
  // Three types of contexts;
  //
  // - A String naming another context
  // - A Vector naming a set of other contextx
  // - An Object with context items (key values)
  //
  // Also, if called with NULL, then all cached contexts are presented
  //

  if (contextP == NULL)
    allCachedContextsPresent();
  else if (contextP->tree == NULL)
  {}
  else if (contextP->tree->type == KjObject)
    objectContextPresent(contextP);
  else if (contextP->tree->type == KjString)
    stringContextPresent(contextP);
  else if (contextP->tree->type == KjArray)
    arrayContextPresent(contextP);
  else
  {
    LM_T(LmtContextPresent, ("%s:", contextP->url));
    LM_T(LmtContextPresent, ("  Invalid Type of tree for a context: %s", kjValueType(contextP->tree->type)));
  }
}
