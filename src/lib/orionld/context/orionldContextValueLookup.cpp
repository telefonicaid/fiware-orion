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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "orionld/common/SCOMPARE.h"                           // SCOMPARE
#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldContextList.h"                // orionldContextHead
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextValueLookup.h"         // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextValueLookupInArray -
//
static KjNode* orionldContextValueLookupInArray(KjNode* contextVector, const char* value);



// -----------------------------------------------------------------------------
//
// orionldContextValueLookupInUrl -
//
static KjNode* orionldContextValueLookupInUrl(char* contextUrl, const char* value)
{
  LM_T(LmtContextValueLookup, ("Looking up '%s' in URI STRING context '%s'", value, contextUrl));

  OrionldContext* contextP = orionldContextLookup(contextUrl);

  if (contextP == NULL)
  {
    LM_E(("Can't find context '%s'", contextUrl));
    return NULL;
  }

  return orionldContextValueLookup(contextP, value);
}



// -----------------------------------------------------------------------------
//
// orionldContextValueLookupInObject -
//
static KjNode* orionldContextValueLookupInObject(KjNode* contextP, const char* value)
{
  LM_T(LmtContextValueLookup, ("Looking up '%s' in OBJECT context '%s'", value, contextP->name));

  if ((contextP->name == NULL) || (strcmp(contextP->name, "@context") == 0))
  {
    LM_T(LmtContextValueLookup, ("Looking up '%s' - looping over items in @context", value));
    for (KjNode* contextItemP = contextP->value.firstChildP; contextItemP != NULL; contextItemP = contextItemP->next)
    {
      if (contextItemP->type == KjString)
      {
        // Skip members whose value is a string that starts with "@" - they are information, not translations
        if (contextItemP->value.s[0] == '@')
        {
          LM_T(LmtContextValueLookup, ("Skipping '%s'", contextItemP->value.s));
          continue;
        }
        LM_T(LmtContextValueLookup, ("Looking for '%s'. Comparing with '%s'", value, contextItemP->value.s));
        if (strcmp(contextItemP->value.s, value) == 0)
          return contextItemP;
      }
      else if (contextItemP->type == KjObject)
      {
        for (KjNode* idNodeP = contextItemP->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
        {
          if (SCOMPARE4(idNodeP->name, '@', 'i', 'd', 0))
          {
            if (strcmp(idNodeP->value.s, value) == 0)
              return contextItemP;
          }
        }
      }
      else
      {
        LM_E(("Invalid @context - items of contexts must be JSON Strings or JSON objects - not %s", kjValueType(contextItemP->type)));
        return NULL;
      }
    }
  }
  else if (strcmp(contextP->value.firstChildP->name, "@context") == 0)
  {
    LM_E(("First child is called '%s' and its type is '%s'", contextP->value.firstChildP->name, kjValueType(contextP->value.firstChildP->type)));
    KjNode* firstChildP = contextP->value.firstChildP;

    if (firstChildP->type == KjArray)
      return orionldContextValueLookupInArray(firstChildP, value);
    else if (firstChildP->type == KjObject)
      return orionldContextValueLookupInObject(firstChildP, value);
    else if (firstChildP->type == KjString)
      return orionldContextValueLookupInUrl(firstChildP->value.s, value);
    else
      LM_E(("invalid type: %s", kjValueType(firstChildP->type)));
  }
  else
  {
    LM_E(("Invalid context: '%s'", contextP->name));
    return NULL;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// orionldContextValueLookupInArray -
//
static KjNode* orionldContextValueLookupInArray(KjNode* contextVector, const char* value)
{
  LM_T(LmtContextValueLookup, ("Looking up '%s' in ARRAY context '%s'", value, contextVector->name));

  for (KjNode* contextNodeP = contextVector->value.firstChildP; contextNodeP != NULL; contextNodeP = contextNodeP->next)
  {
    KjNode* itemP;

    if (contextNodeP->type == KjString)
      itemP = orionldContextValueLookupInUrl(contextNodeP->value.s, value);
    else if (contextNodeP->type == KjObject)
      itemP = orionldContextValueLookupInObject(contextNodeP, value);
    else
    {
      LM_E(("Invalid array member - must be String or Object, not '%s'", kjValueType(contextNodeP->type)));
      return NULL;
    }

    if (itemP != NULL)
      return itemP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// orionldContextValueLookup -
//
KjNode* orionldContextValueLookup(OrionldContext* contextP, const char* value)
{
  if (contextP == NULL)
    contextP = &orionldCoreContext;

  LM_T(LmtContextValueLookup, ("Looking up '%s' in context '%s' (which is of type '%s')", value, contextP->url, kjValueType(contextP->tree->type)));

  if (contextP->tree->type == KjString)
    return orionldContextValueLookupInUrl(contextP->tree->value.s, value);
  else if (contextP->tree->type == KjArray)
    return orionldContextValueLookupInArray(contextP->tree, value);
  else if (contextP->tree->type == KjObject)
    return orionldContextValueLookupInObject(contextP->tree, value);

  LM_E(("Error: contexts must be either a String, an Object or an Array"));
  return NULL;
}
