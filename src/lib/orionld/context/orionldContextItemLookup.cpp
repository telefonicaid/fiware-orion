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

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldContextList.h"                // orionldContextHead
#include "orionld/context/orionldContextListPresent.h"         // orionldContextListPresent
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextCreateFromUrl.h"       // orionldContextCreateFromUrl
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

  LM_TMP(("VEX: Looking up '%s' in context '%s'", itemName, contextP->url));

  if (contextP->ignore == true)
  {
    LM_T(LmtContextItemLookup, ("The context '%s' is ignored - ContextItem '%s' not found", contextP->url, itemName));
    LM_TMP(("CTX: context '%s' is ignored", contextP->url));
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
    LM_TMP(("CTX: Recursively calling orionldContextItemLookup(%s, %s)", dereferencedContextP->url, itemName));
    KjNode* nodeP = orionldContextItemLookup(dereferencedContextP, itemName);
    LM_TMP(("CTX: orionldContextItemLookup returned %p", nodeP));
    return nodeP;
  }
  else if (contextP->tree->type == KjArray)
  {
    LM_TMP(("CTX: The context '%s' is of type Array", contextP->url));
    LM_T(LmtContextItemLookup, ("The context '%s' is of type Array", contextP->url));

    for (KjNode* contextNodeP = contextP->tree->value.firstChildP; contextNodeP != NULL; contextNodeP = contextNodeP->next)
    {
      LM_TMP(("CTX: Array Item is of JSON Type '%s'", kjValueType(contextNodeP->type)));
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
      LM_TMP(("CTX: Recursively calling orionldContextItemLookup for the array item '%s'", dereferencedContextP->url));
      KjNode* kNodeP = orionldContextItemLookup(dereferencedContextP, itemName);
      LM_TMP(("CTX: orionldContextItemLookup returned %p", kNodeP));
      if (kNodeP != NULL)
      {
        LM_TMP(("CTX: Found longname '%s' for shortname '%s'", kNodeP->value.s, itemName));
        LM_T(LmtContextItemLookup, ("Found '%s' in context '%s'", itemName, dereferencedContextP->url));
        return kNodeP;
      }
    }

    LM_T(LmtContextItemLookup, ("Context Item '%s' - NOT FOUND", itemName));
    LM_TMP(("CTX: Context Item '%s' - NOT FOUND", itemName));
    return NULL;
  }
  else if (contextP->tree->type == KjObject)
  {
    LM_TMP(("CTX: Context is an object - but ... is @context a member, etc?"));

    for (KjNode* nodeP = contextP->tree->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (strcmp(nodeP->name, "@context") == 0)
      {
        LM_TMP(("CTX: Found a @context member - calling recursively ..."));
        if (nodeP->type == KjString)
        {
          KjNode* hitP;
          if ((hitP = orionldContextItemLookup(nodeP->value.s, itemName)) != NULL)
            return hitP;
        }
        else if (nodeP->type == KjArray)
        {
          for (KjNode* itemP = nodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
          {
            KjNode* hitP;
            if ((hitP = orionldContextItemLookup(itemP->value.s, itemName)) != NULL)
              return hitP;
          }
        }
      }
    }

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
  LM_TMP(("VEX: Time to find the item in the context that is a JSON Object of key-values"));
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
    //
    // Might be an inline context. If so, the atContextP is already an item of the context ...
    // FIXME: What a mess!
    //
    // If inline context, then chances are that the name of the first key DOES NOT start with '@'.
    // Also, if looked up, we would not find a member called "@context".
    //
    if (atContextP->name[0] != '@')
    {
      //
      // Assuming we're in an inline context
      //
      while (atContextP != NULL)
      {
        if (strcmp(itemName, atContextP->name) == 0)
        {
          if (atContextP->type == KjString)
            LM_TMP(("VEX: Found '%s'. Long name: '%s'", itemName, atContextP->value.s));
          else
            LM_TMP(("VEX: Found '%s' in a context item that is an object", itemName));

          return atContextP;
        }

        atContextP = atContextP->next;
      }

      LM_TMP(("VEX: '%s' not found in inline context", itemName));
      return NULL;
    }

    LM_TMP(("VEX: @context item '%s' is an object", atContextP->name));
    LM_T(LmtContextItemLookup, ("@context '%s' is an object", atContextP->name));
    LM_T(LmtContextItemLookup, ("Looking for '%s', atContextP is named '%s'", itemName, atContextP->name));

    if (strcmp(itemName, atContextP->name) == 0)
    {
      LM_TMP(("VEX: Found '%s'", itemName));
      return atContextP;
    }

    int ix = 0;
    for (KjNode* contextItemP = atContextP->value.firstChildP; contextItemP != NULL; contextItemP = contextItemP->next)
    {
      if ((contextItemP->type != KjString) && (contextItemP->type != KjObject))
      {
        LM_E(("Invalid @context - items of contexts must be JSON Strings or JSON objects - not %s", kjValueType(contextItemP->type)));
        return NULL;
      }
      LM_T(LmtContextItemLookup, ("@context '%s' item %d: name: '%s'", atContextP->name, ix, contextItemP->name));

      //
      // Skip members whose value is a string that starts with "@", except for "@type"
      //
      LM_T(LmtContextItemLookup, ("Looking for '%s', current context item is named '%s'", itemName, contextItemP->name));
      if ((contextItemP->type == KjString) && (contextItemP->value.s[0] == '@') && (strcmp(contextItemP->value.s, "@type") != 0))
      {
        LM_T(LmtContextItemLookup, ("Skipping '%s' with value '%s'", contextItemP->name, contextItemP->value.s));
        continue;
      }

      LM_T(LmtContextItemLookup, ("looking for '%s', comparing with '%s'", itemName, contextItemP->name));
      if (strcmp(contextItemP->name, itemName) == 0)
      {
        if (contextItemP->type == KjString)
          LM_T(LmtContextItemLookup, ("found it! '%s' -> '%s'", itemName, contextItemP->value.s));
        else
          LM_T(LmtContextItemLookup, ("found it! ('%s') it's a JSON %s'", itemName, kjValueType(contextItemP->type)));

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

      LM_TMP(("CTX: Recursively calling orionldContextItemLookup(context='%s', searchItem='%s')", contextP->value.s, itemName));
      KjNode* nodeP = orionldContextItemLookup(contextP->value.s, itemName);
      LM_TMP(("CTX: orionldContextItemLookup returned %p for '%s' in context '%s'", nodeP, itemName, contextP->value.s));

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
  LM_TMP(("CTX: found no expansion for '%s': returning NULL :(", itemName));
  return NULL;
}




// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
KjNode* orionldContextItemLookup(char* contextUrl, const char* itemName)
{
  LM_TMP(("CTX: looking up context '%s'", contextUrl));
  OrionldContext* contextP = orionldContextLookup(contextUrl);

  if (contextP == NULL)
  {
    char* details;

    LM_TMP(("CTX: did not find context '%s', while searching for '%s' - downloading it!!!", contextUrl, itemName));
    contextP = orionldContextCreateFromUrl(orionldState.ciP, contextUrl, OrionldUserContext, &details);

    if (contextP == NULL)
      return NULL;
  }

  LM_TMP(("CTX: calling orionldContextItemLookup for context '%s', searching for '%s'", contextUrl, itemName));
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
