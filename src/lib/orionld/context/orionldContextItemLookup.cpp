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

#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldContextList.h"                // orionldContextHead
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextItemLookup.h"          // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
// If the context 'contextP' comes from the payload, then it can be:
// - A JSON String, naming another Context
// - A JSON Vector of JSON Strings, naming a group of contexts
// - A JSON Object, being an inline context (not supported right now)
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
  LM_TMP(("contextP at %p", contextP));
  if (contextP == NULL)
    return NULL;
  LM_TMP(("context URL: %s", contextP->url));
  LM_TMP(("contextP->tree at %p", contextP->tree));
  if (contextP->tree->type == KjString)
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
      {
        LM_T(LmtContextItemLookup, ("Found '%s' in context '%s'", itemName, dereferencedContextP->url));
        return kNodeP;
      }
    }

    return NULL;
  }
  else if (contextP->tree->type == KjObject)
  {
    //
    // JSON Object - only check here. Do the lookup later.
    // this way, the default context gets the lookup as well
    //
    // Now, either we have an inline context, with many key-values, or
    // the object has a single member "@context".
    //
    LM_T(LmtContextItemLookup, ("Looking up '%s' in the %s context '%s'", itemName, kjValueType(contextP->tree->type), contextP->url));

    if (contextP->tree->children == NULL)
    {
      LM_E(("Context tree is a JSON object, but, it has no children!"));
      return NULL;
    }
#if 0    
    if (contextP->tree->children->next != NULL)
    {
      LM_E(("Context tree is a JSON object, but, it has more than one child! (%s)", contextP->url));
      return NULL;
    }
    if (strcmp(contextP->tree->children->name, "@context") != 0)
    {
      LM_E(("Context tree is a JSON object, and it has exactly one child, but its name must be '@context', not '%s'", contextP->tree->children->name));
      return NULL;
    }
#endif
  }
  else
  {
    LM_E(("The '@context' is a %s - must be either Object, String or Array", kjValueType(contextP->tree->children->type)));
    return NULL;
  }


  //
  // If we reach this point, the context is a JSON Object
  //
  KjNode*  atContextP    = contextP->tree->children;

  LM_T(LmtContextItemLookup, ("@atcontext is of type '%s'", kjValueType(atContextP->type)));

  //
  // About contextP->tree->children->children:
  //
  // If a KjObject, the payload looks like this:
  // {
  //   "@context": {
  //     "xxx"; "http://...",
  //     ...
  //   }
  // }
  //
  // Now, the tree (contextP->tree) points at the start of the payload (the very first '{')
  // It's first (and only) child is "@context", whose first child is "xxx".
  // Thus, contextP->tree->children->children references "xxx".
  //
  // The "xxx" child has siblings following the next pointer until reaching NULL
  //
  if (atContextP->type == KjObject)
  {
    for (KjNode* contextItemP = atContextP->children; contextItemP != NULL; contextItemP = contextItemP->next)
    {
      if ((contextItemP->type != KjString) && (contextItemP->type !=  KjObject))
      {
        LM_E(("Invalid @context - items of contexts must be JSON Strings or jSOn objects - not %s", kjValueType(contextItemP->type)));
        return NULL;
      }

      //
      // Skip members whose value is a string that starts with "@" - they are information, not translations
      //
      if ((contextItemP->type == KjString) && (contextItemP->value.s[0] == '@'))
      {
        LM_T(LmtContextItemLookup, ("Skipping '%s' with value '%s'", contextItemP->name, contextItemP->value.s));
        continue;
      }

      // LM_T(LmtContextItemLookup, ("looking for '%s', comparing with '%s'", itemName, contextItemP->name));
      if (strcmp(contextItemP->name, itemName) == 0)
      {
        LM_T(LmtContextItemLookup, ("found it!"));
        return contextItemP;
      }
    }
  }
  else if (atContextP->type == KjArray)
  {
    for (KjNode* contextP = atContextP->children; contextP != NULL; contextP = contextP->next)
    {
      if (contextP->type != KjString)
      {
        LM_E(("Invalid @context - items of contexts must be JSON Strings"));
        return NULL;
      }

      KjNode* nodeP = orionldContextItemLookup(contextP->value.s, itemName);

      if (nodeP != NULL)
        return nodeP;
    }
  }
  
  LM_T(LmtContextItemLookup, ("found no expansion: returning NULL :("));
  return NULL;
}




// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
KjNode* orionldContextItemLookup(char* contextUrl, const char* itemName)
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
KjNode* orionldContextItemLookup(KjNode* contextVector, const char* itemName)
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
