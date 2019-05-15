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
#include "orionld/context/orionldContextListPresent.h"         // orionldContextListPresent
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
  if (contextP == NULL)
  {
    LM_T(LmtContextItemLookup, ("The context is a NULL pointer - ContextItem '%s' not found", itemName));
    return NULL;
  }

  if (contextP->ignore == true)
  {
    LM_T(LmtContextItemLookup, ("The context '%s' is ignored - ContextItem '%s' not found", contextP->url, itemName));
    return NULL;
  }

  if (contextP->tree == NULL)
  {
    LM_T(LmtContextItemLookup, ("The context '%s' HAS NO CONTENT TREE - ContextItem '%s' not found", contextP->url, itemName));
    return NULL;
  }

  LM_T(LmtContextItemLookup, ("Looking up ContextItem '%s' in context '%s' that is of type '%s', contextTree at %p", itemName, contextP->url, kjValueType(contextP->tree->value.firstChildP->type), contextP->tree));
  if (contextP->tree->value.firstChildP->type > 7)
  {
    LM_E(("Context '%s' seems corrupted", contextP->url));
    LM_E(("Tree at: %p", contextP->tree));
    LM_E(("firstChild at %p", contextP->tree->value.firstChildP));
    LM_E(("node type of firstChild: %d", contextP->tree->value.firstChildP->type));
    exit(21);
  }

  if (contextP->tree->type == KjString)
  {
    LM_T(LmtContextItemLookup, ("The context '%s' is of type String - must lookup a new context (%s)", contextP->url, contextP->tree->value.s));
    orionldContextListPresent();
    // Lookup the context
    OrionldContext* dereferencedContextP = orionldContextLookup(contextP->tree->value.s);
    if (dereferencedContextP == NULL)
    {
      LM_E(("Can't find context '%s'", contextP->tree->value.s));
      return NULL;  // Or: download it?
    }

    // Lookup the item
    LM_T(LmtContextItemLookup, ("Found the context '%s' - now we can search for '%s' (in '%s')", contextP->url, itemName, dereferencedContextP->url));
    return orionldContextItemLookup(dereferencedContextP, itemName);
  }
  else if (contextP->tree->type == KjArray)
  {
    LM_T(LmtContextItemLookup, ("The context '%s' is of type Array", contextP->url));

    for (KjNode* contextNodeP = contextP->tree->value.firstChildP; contextNodeP != NULL; contextNodeP = contextNodeP->next)
    {
      if (contextNodeP->type != KjString)
      {
        LM_E(("ERROR - the Context Array must only contain JSON Strings"));
        return NULL;
      }

      LM_T(LmtContextItemLookup, ("Calling orionldContextLookup to lookup context '%s'", contextNodeP->value.s));

      OrionldContext* dereferencedContextP = orionldContextLookup(contextNodeP->value.s);

      if (dereferencedContextP == NULL)
      {
        LM_W(("Can't find context '%s' - trying with the next context in the Array", contextNodeP->value.s));
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

    LM_T(LmtContextItemLookup, ("Context Item '%s' - NOT FOUND", itemName));
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
    LM_T(LmtContextItemLookup, ("NOT Looking up '%s' in the %s context '%s' - it's done later", itemName, kjValueType(contextP->tree->value.firstChildP->type), contextP->url));

    if (contextP->tree->value.firstChildP == NULL)
    {
      LM_E(("ERROR: Context tree is a JSON object, but, it has no children!"));
      return NULL;
    }
  }
  else if (contextP->tree->value.firstChildP == NULL)
  {
    LM_E(("ERROR: NULL context tree '%s'", contextP->url));
    return NULL;
  }
  else
  {
    LM_E(("ERROR: invalid context tree '%s' - not sure why", contextP->url));
    return NULL;
  }

  //
  // If we reach this point, the context is a JSON Object
  //
  LM_T(LmtContextItemLookup, ("Lookup of item '%s' in JSON Object Context '%s'", itemName, contextP->url));

  KjNode* atContextP = contextP->tree->value.firstChildP;

  //
  // About contextP->tree->value.firstChildP->value.firstChildP:
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
  // Thus, contextP->tree->value.firstChildP->value.firstChildP references "xxx".
  //
  // The "xxx" child has siblings following the next pointer until reaching NULL
  //
  LM_T(LmtContextItemLookup, ("atContextP->type == %s", kjValueType(atContextP->type)));
  if (atContextP->type == KjObject)
  {
    LM_T(LmtContextItemLookup, ("@context '%s' is an object", atContextP->name));

    for (KjNode* contextItemP = atContextP->value.firstChildP; contextItemP != NULL; contextItemP = contextItemP->next)
    {
      if ((contextItemP->type != KjString) && (contextItemP->type != KjObject))
      {
        LM_E(("Invalid @context - items of contexts must be JSON Strings or JSON objects - not %s", kjValueType(contextItemP->type)));
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
        LM_T(LmtContextItemLookup, ("found it! '%s' -> '%s'", itemName, contextItemP->value.s));
        return contextItemP;
      }
    }
  }
  else if (atContextP->type == KjArray)  // FIXME: What is this? Can we really get here ... ?
  {
    for (KjNode* contextP = atContextP->value.firstChildP; contextP != NULL; contextP = contextP->next)
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
  else if (atContextP->type == KjString)
  {
    for (KjNode* contextItemP = atContextP; contextItemP != NULL; contextItemP = contextItemP->next)
    {
      if ((contextItemP->type != KjString) && (contextItemP->type != KjObject))
      {
        LM_E(("Invalid @context - items of contexts must be JSON Strings or JSON objects - not %s", kjValueType(contextItemP->type)));
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
        LM_T(LmtContextItemLookup, ("found it! '%s' -> '%s'", itemName, contextItemP->value.s));
        return contextItemP;
      }
    }
  }

  LM_T(LmtContextItemLookup, ("found no expansion for '%s': returning NULL :(", itemName));
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

  KjNode* contextNodeP = contextVector->value.firstChildP;
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
