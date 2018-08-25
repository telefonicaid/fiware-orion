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
#include "logMsg/logMsg.h"

extern "C"
{
#include "kjson/KjNode.h"                              // KjNode
}

#include "orionld/context/OrionldContext.h"            // OrionldContext
#include "orionld/context/orionldContextList.h"        // orionldContextHead
#include "orionld/context/orionldContextPresent.h"     // Own interface



// ----------------------------------------------------------------------------
//
// allCachedContextsPresent -
//
static void allCachedContextsPresent(void)
{
  OrionldContext* contextP = orionldContextHead;

  while (contextP != NULL)
  {
    orionLdContextPresent(contextP);
    contextP = contextP->next;
  }
}



// ----------------------------------------------------------------------------
//
// stringContextPresent -
//
static void stringContextPresent(OrionldContext* contextP)
{
  LM_TMP(("String: %s:", contextP->url));
  LM_TMP(("  %s", contextP->tree->value.s));
}



// ----------------------------------------------------------------------------
//
// arrayContextPresent -
//
static void arrayContextPresent(OrionldContext* contextP)
{
  KjNode* itemP;
    
  LM_TMP(("Array: %s:", contextP->url));

  for (itemP = contextP->tree->children; itemP != NULL; itemP = itemP->next)
  {
    LM_TMP(("  %s", itemP->value.s));
  }
}



// ----------------------------------------------------------------------------
//
// objectContextPresent -
//
static void objectContextPresent(OrionldContext* contextP)
{
  LM_TMP(("Object: %s:", contextP->url));

  
  for (KjNode* nodeP = contextP->tree->children->children; nodeP != NULL; nodeP = nodeP->next)
  {
    if (nodeP->type == KjString)
      LM_TMP(("  %s: %s", nodeP->name, nodeP->value.s));
    else if (nodeP->type == KjObject)
    {
      LM_TMP(("  %s: {", nodeP->name));

      for (KjNode* childP = nodeP->children; childP != NULL; childP = childP->next)
        LM_TMP(("    %s: %s", childP->name, childP->value.s));
      LM_TMP(("  }"));
    }
  }
}



// ----------------------------------------------------------------------------
//
// orionLdContextPresent -
//
void orionLdContextPresent(OrionldContext* contextP)
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
  else if (contextP->tree->type == KjObject)
    objectContextPresent(contextP);
  else if (contextP->tree->type == KjString)
    stringContextPresent(contextP);
  else if (contextP->tree->type == KjArray)
    arrayContextPresent(contextP);
  else
  {
    LM_TMP(("%s:", contextP->url));
    LM_TMP(("  Invalid Type of tree for a context: %s", kjValueType(contextP->tree->type)));
  }
}
