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
#include "kjson/KjNode.h"                               // KjNode
}

#include "orionld/context/OrionldContext.h"               // OrionldContext
#include "orionld/context/orionldDefaultContext.h"        // orionldDefaultContext
#include "orionld/context/orionldContextList.h"           // orionldContextHead
#include "orionld/context/orionldContextLookup.h"         // orionldContextLookup
#include "orionld/context/orionldContextItemLookup.h"     // Own interface


// Temporarily removing all TMP traces in this file
// #undef LM_TMP
// #define LM_TMP(s)



// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
KjNode* orionldContextItemLookup(OrionldContext* contextP, const char* itemName)
{
  //
  // A context must be a tree object and inside the tree there must be only
  // String values or Object values
  //
  // There are three different contexts:
  // 1. Simple contexts: a KjArray
  // 2. Dereferenced contexts: a KjString naming the context
  // 3. Complex contexts: a KjObject containing a KjVector (called '@context') with dereferenced contexts
  //
  // Also, if the context is NULL, then the default context is used in its place.
  //
  if (contextP == NULL)
  {
    LM_TMP(("context is NULL, using the default context '%s'", orionldDefaultContext.url));
    contextP = &orionldDefaultContext;
  }

  LM_TMP(("uriExpansion: Looking up '%s' in the context '%s'", itemName, contextP->url));

  if (contextP->tree->type == KjArray)
  {
    LM_TMP(("uriExpansion: The context is of type Array"));

    for (KjNode* contextNodeP = contextP->tree->children; contextNodeP != NULL; contextNodeP = contextNodeP->next)
    {
      OrionldContext* vecContextP = orionldContextLookup(contextNodeP->value.s);

      if (vecContextP == NULL)
      {
        LM_W(("Can't find context '%s'", contextNodeP->value.s));
        continue;   // Or: download it?
      }

      // Lookup the item
      LM_TMP(("uriExpansion: Now we can search for '%s' in context '%s'", itemName, vecContextP->url));
      KjNode* kNodeP = orionldContextItemLookup(vecContextP, itemName);

      if (kNodeP != NULL)
        return kNodeP;
    }

    return NULL;
  }
  else if (contextP->tree->type == KjString)
  {
    LM_TMP(("uriExpansion: The context is of type String - must lookup a new context"));
    // Lookup the context
    contextP = orionldContextLookup(contextP->tree->value.s);
    if (contextP == NULL)
      return NULL;  // Or: download it?

    // Lookup the item
    LM_TMP(("uriExpansion: Now we can search for '%s'", itemName));
    return orionldContextItemLookup(contextP, itemName);
  }
  else if (contextP->tree->type == KjObject)
  {
    LM_TMP(("uriExpansion: The context is of type Object"));
    int contextItemNo = 0;  // TMP
    for (KjNode* contextItemP = contextP->tree->children->children; contextItemP != NULL; contextItemP = contextItemP->next)
    {
      // <TMP>
      LM_TMP(("contextItemNo: %d at %p (name: '%s')", contextItemNo, contextItemP, contextItemP->name));
      ++contextItemNo;
      // </TMP>
      
      //
      // Skip members whose value starts with "@" - they are information, not translations
      //
      if ((contextItemP->type == KjString) && (contextItemP->value.s[0] == '@'))
      {
        LM_TMP(("Skipping '%s' with value '%s'", contextItemP->name, contextItemP->value.s));
        continue;
      }
      
      LM_TMP(("uriExpansion: looking for '%s', comparing with '%s'", itemName, contextItemP->name));
      if (strcmp(contextItemP->name, itemName) == 0)
      {
        LM_TMP(("uriExpansion: found it!"));
        return contextItemP;
      }
    }
  }

  LM_TMP(("uriExpansion: found no expansion: returning NULL :("));
  return NULL;
}




// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
KjNode* orionldContextItemLookup(char* contextUrl, char* itemName)
{
  OrionldContext* contextP = orionldContextLookup(contextUrl);

  if (contextP == NULL)
    return NULL;

  return orionldContextItemLookup(contextP, itemName);
}



// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
KjNode* orionldContextItemLookup(KjNode* contextVector, char* itemName)
{
  if (contextVector->type != KjArray)
  {
    LM_E(("Not an Array"));
    return NULL;
  }

  KjNode* contextNodeP = contextVector->children;
  KjNode* itemP;

  while (contextNodeP != NULL)
  {
    OrionldContext* contextP = orionldContextLookup(contextNodeP->value.s);

    if (contextP == NULL)
      return NULL;

    itemP = orionldContextItemLookup(contextP, itemName);
    if (itemP != NULL)
      return itemP;

    contextNodeP = contextNodeP->next;
  }

  return NULL;
}



