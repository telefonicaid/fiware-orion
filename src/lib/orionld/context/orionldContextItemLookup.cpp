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
#include "logMsg/logMsg.h"                                  // LM_*
#include "logMsg/traceLevels.h"                             // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                   // KjNode
}

#include "orionld/context/OrionldContext.h"                 // OrionldContext
#include "orionld/context/orionldDefaultContext.h"          // orionldDefaultContext
#include "orionld/context/orionldContextList.h"             // orionldContextHead
#include "orionld/context/orionldContextLookup.h"           // orionldContextLookup
#include "orionld/context/orionldContextItemLookup.h"       // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
// If the context 'contextP' comes from the payload, then it can be:
// - A JSON String, naming another Context
// - A JSON Vector of JSON Strings, naming a group of contexts
//
// If instead it is a downloaded context, then the context 'contextP'  must always be:
//   - A JSON Object
//   - With ONE and only ONE child
//   - The child must be named '@context'
//   - The '@context' child can be;
//     - A JSON Array of JSON Strings, naming other contexts
//     - A JSON String, naming another Context
//     - A JSON Object, with key-value pairs (this is the 'final' context where the info resides)
//
// This function is called from:
//   o serviceRoutines - with an "entity context" as first parameter
//   o orionldContextItemLookup, recursively, with ...
//
KjNode* orionldContextItemLookup(OrionldContext* contextP, const char* itemName)
{
  LM_TMP(("========== Looking up '%s' in context '%s'", itemName, contextP->url));
  LM_TMP(("========== The context tree is of type %s", kjValueType(contextP->tree->type)));

  if (contextP->tree->type == KjObject)
    LM_TMP(("========== First member is called '%s'", contextP->tree->children->name));

//  LM_TMP(("========== "));
//  LM_TMP(("========== "));

  if (contextP == NULL)
  {
    LM_T(LmtContextItemLookup, ("context is NULL, using the default context '%s'", orionldDefaultContext.url));
    contextP = &orionldDefaultContext;
  }
  else if (contextP->tree->type == KjString)
  {
    LM_T(LmtContextItemLookup, ("The context is of type String - must lookup a new context"));

    // Lookup the context
    OrionldContext* dereferencedContextP = orionldContextLookup(contextP->tree->value.s);
    if (dereferencedContextP == NULL)
    {
      LM_E(("Can't find context '%s'", contextP->tree->value.s));
      return NULL;  // Or: download it?
    }

    // Lookup the item
    LM_T(LmtContextItemLookup, ("Now we can search for '%s' (in context '%s'", itemName, dereferencedContextP->url));
    return orionldContextItemLookup(dereferencedContextP, itemName);
  }
  else if (contextP->tree->type == KjArray)
  {
    LM_T(LmtContextItemLookup, ("The context is of type Array"));

    for (KjNode* contextNodeP = contextP->tree->children; contextNodeP != NULL; contextNodeP = contextNodeP->next)
    {
      if (contextNodeP->type != KjString)
      {
        LM_E(("The Context Array must only contain JSON Strings"));
        return NULL;
      }

      OrionldContext* dereferencedContextP = orionldContextLookup(contextNodeP->value.s);

      if (dereferencedContextP == NULL)
      {
        LM_W(("Can't find context '%s'", contextNodeP->value.s));
        continue;   // Or: download it?
      }

      // Lookup the item
      LM_T(LmtContextItemLookup, ("Now we can search for '%s' in context '%s'", itemName, dereferencedContextP->url));
      KjNode* kNodeP = orionldContextItemLookup(dereferencedContextP, itemName);

      if (kNodeP != NULL)
        return kNodeP;
    }

    return NULL;
  }
  else if (contextP->tree->type == KjObject)
  {
    //
    // JSON Object, either:
    // {
    //   "@context":
    // }
    //
    LM_T(LmtContextItemLookup, ("Looking up '%s' in the %s context '%s'", itemName, kjValueType(contextP->tree->type), contextP->url));

    if (contextP->tree->children == NULL)
    {
      LM_E(("Context tree is a JSON object, but, it has no children!"));
      return NULL;
    }
    if (contextP->tree->children->next != NULL)
    {
      LM_E(("Context tree is a JSON object, but, it has more than one child!"));
      return NULL;
    }
    if (strcmp(contextP->tree->children->name, "@context") != 0)
    {
      LM_E(("Context tree is a JSON object, and it has exactly one child, but its name must be '@context', not '%s'", contextP->tree->children->name));
      return NULL;
    }
  }
  else
  {
    LM_E(("The '@context' object is a %s - must be either Object, String or Array", kjValueType(contextP->tree->children->type)));
    return NULL;
  }

  KjNode* atContextP = contextP->tree->children;

  if (atContextP->type == KjArray)
  {
  }
  else if (atContextP->type == KjObject)
  {
    LM_T(LmtContextItemLookup, ("The context is of type Object"));
    int contextItemNo = 0;  // TMP



    //
    // About contextP->tree->children->children:
    //
    // If an KjObject, the payload looks like this:
    // {
    //   "@context": {
    //     "xxx"; "http://...",
    //     ...
    //   }
    // }
    //
    // Now, the tree (contextP->tree) points ti the start of the payload (the very firat '{')
    // It's first (and only) child is "@context", whose first child is "xxx".
    // Thus contextP->tree->children->children.
    //
    // The "xxx" child has siblings following the next pointer until reaching NULL
    //
    for (KjNode* contextItemP = atContextP->children; contextItemP != NULL; contextItemP = contextItemP->next)
    {
      if ((contextItemP->type != KjString) && (contextItemP->type != KjObject))
      {
        LM_E(("Invalid @context - items must be JSON Strings or JSON Objects"));
        return NULL;
      }

      // <TMP>
      LM_T(LmtContextItemLookup, ("contextItemNo: %d at %p (name: '%s')", contextItemNo, contextItemP, contextItemP->name));
      ++contextItemNo;
      // </TMP>

      //
      // Skip members whose value is a string that starts with "@" - they are information, not translations
      //
      if ((contextItemP->type == KjString) && (contextItemP->value.s[0] == '@'))
      {
        LM_T(LmtContextItemLookup, ("Skipping '%s' with value '%s'", contextItemP->name, contextItemP->value.s));
        continue;
      }

      LM_T(LmtContextItemLookup, ("looking for '%s', comparing with '%s'", itemName, contextItemP->name));
      if (strcmp(contextItemP->name, itemName) == 0)
      {
        LM_T(LmtContextItemLookup, ("found it!"));
        return contextItemP;
      }
    }
  }

  LM_T(LmtContextItemLookup, ("found no expansion: returning NULL :("));
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



