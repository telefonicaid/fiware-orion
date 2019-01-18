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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/SCOMPARE.h"                           // SCOMPARE4
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextValueLookup.h"         // orionldContextValueLookup
#include "orionld/context/orionldUserContextKeyValuesCheck.h"  // Own interface



// -----------------------------------------------------------------------------
//
// orionldUserContextKeyValuesCheck2 -
//
// - 'tree' must be a KjObject
// - All members of 'tree' must be strings OR objects
//   - If Object, all members must be strings. "@id" is mandatory and "@type" is OK. Rest is ignored
// - No value of the strings can be found as a value in the Core context
// - The key "@vocab" is forbidden
//
// NOTE
//   The tree must be the value of a "@context" node - an object containing key-values.
//   E.g.:
//   {
//     "@context":
//     {                       <----------- this object is what 'tree' must point to
//       "alias1": "value1",
//       "alias1": "value2",
//       ...,
//       "aliasN": "valueN",
//     }
//   }
static bool orionldUserContextKeyValuesCheck2(KjNode* tree, char* url, char** detailsPP)
{
  if (tree->type != KjObject)
  {
    LM_X(1, ("Context Tree is not a JSON Object - this is a bug!"));
    *detailsPP = (char*) "not a JSON Object";
    return false;
  }

  for (KjNode* childP = tree->value.firstChildP; childP != NULL; childP = childP->next)
  {
    if (SCOMPARE7(childP->name, '@', 'v', 'o', 'c', 'a', 'b', 0))
    {
      LM_E(("In context '%s': the use of '@vocab' is prohibited", url));
      *detailsPP = (char*) "Invalid context - the use of '@vocab' is prohibited";
      return false;
    }
    if (childP->type == KjString)
    {
      bool useStringValue = false;

      if (orionldContextValueLookup(&orionldCoreContext, childP->value.s, &useStringValue) != NULL)
      {
        LM_E(("In context '%s', the context item '%s' uses a value from the Core Context (%s)",
              url, childP->name, childP->value.s));
        *detailsPP = (char*) "Invalid context - values from the Core Context cannot be used";
        return false;
      }
    }
    else if (childP->type == KjObject)
    {
      KjNode* atidP = NULL;

      for (KjNode* itemP = childP->value.firstChildP; itemP != NULL; itemP = itemP->next)
      {
        if (itemP->type != KjString)
        {
          LM_E(("In context '%s', the context item '%s' is not a JSON String", url, itemP->name));
          *detailsPP = (char*) "Invalid context - item is not a String";
          return false;
        }

        if (SCOMPARE4(itemP->name, '@', 'i', 'd', 0))
        {
          bool useStringValue = false;

          atidP = itemP;
          LM_T(LmtContext, ("Checking value '%s' for iten named '%s'", itemP->value.s, itemP->name));
          if (orionldContextValueLookup(&orionldCoreContext, itemP->value.s, &useStringValue) != NULL)
          {
            LM_E(("In context '%s', the context item '%s' of '%s' uses a value from the Core Context (%s)",
                  url, itemP->name, childP->name, itemP->value.s));
            *detailsPP = (char*) "Invalid context - values from the Core Context cannot be used";
            return false;
          }
        }
      }

      if (atidP == NULL)
      {
        LM_E(("In context '%s', the the object contest item '%s' has no '@id' member", url, childP->name));
        *detailsPP = (char*) "Invalid context - no @id member found in object context item";
        return false;
      }
    }
    else
    {
      *detailsPP = (char*) "Invalid context - context items must be either strings or objects";
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldUserContextKeyValuesCheck -
//
// 6 types of KjTree:
//   1. String in Object:
//      {
//        "@context": "URL"
//      }
//
//   2. Array in Object:
//      {
//        "@context": [ "URL1", "URL2", ..., "URL-N" ]
//      }
//
//   3. Object in Object:
//      {
//        "@context": {
//          "alias1": "value1",
//          "alias2", "value2",
//          ...,
//          "aliasN": "valueN"
//        }
//      }
//
//   4. Simple String:
//      "URL"
//
//   5. Simple Vector:
//      [ "URL1", "URL2", ..., "URL-N" ]
//
//   6. Simple Object:
//      {
//        "alias1": "value1",
//        "alias2", "value2",
//        ...,
//        "aliasN": "valueN"
//      }
//
// FIXME: The idea is to use only cases 5 and 6.
//        If any other case, convert to 5 or 6
//
bool orionldUserContextKeyValuesCheck(KjNode* contextTreeP, char* url, char** detailsPP)
{
  if (contextTreeP == NULL)
  {
    *detailsPP = (char*) "NULL context tree";
    return NULL;
  }

  if (contextTreeP->value.firstChildP == NULL)
  {
    LM_E(("contextTreeP '%s' is of type '%s' (next at %p)",
          url, kjValueType(contextTreeP->type), contextTreeP->name, contextTreeP->next));
    *detailsPP = (char*) "Context tree is empty";
    return NULL;
  }

  //
  // Cases 1-3 are KjObject having one single member called "@context"
  // Case 6 is an object as well, but it normally has more than one member and none of them called "@context"
  //
  OrionldContext* deferredContextP;

  if (contextTreeP->type == KjObject)
  {
    // Case 1-3
    if ((contextTreeP->value.firstChildP->next == NULL) &&
        SCOMPARE9(contextTreeP->value.firstChildP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
    {
      KjNode* contextNodeP = contextTreeP->value.firstChildP;

      if (contextTreeP->value.firstChildP->type == KjObject)
      {
        return orionldUserContextKeyValuesCheck2(contextTreeP->value.firstChildP, url, detailsPP);
      }
      else if (contextNodeP->type == KjString)
      {
        deferredContextP = orionldContextLookup(contextNodeP->value.s);
        if (deferredContextP == NULL)
        {
          LM_E(("Context '%s' of string context '%s' not found", contextNodeP->value.s, url));
          *detailsPP = (char*) "context not found";
          return false;
        }

        return orionldUserContextKeyValuesCheck(deferredContextP->tree, deferredContextP->url, detailsPP);
      }
      else if (contextNodeP->type == KjArray)
      {
        for (KjNode* itemP = contextNodeP; itemP != NULL; contextNodeP = contextNodeP->next)
        {
          if (orionldUserContextKeyValuesCheck(itemP, itemP->name, detailsPP) == false)
            return false;
        }
      }
      else  // Invalid JSON type of "@context" member (Not Object, Array, nor String)
      {
        LM_E(("Invalid JSON type of '@context' member"));
        *detailsPP = (char*) "Invalid JSON type of '@context' member";
        return false;
      }
    }
    else  // Case 6 - Simple Object
    {
      return orionldUserContextKeyValuesCheck2(contextTreeP, url, detailsPP);
    }
  }
  else if (contextTreeP->type == KjString)
  {
    deferredContextP = orionldContextLookup(contextTreeP->value.s);
    if (deferredContextP == NULL)
    {
      LM_E(("Context '%s' of string context '%s' not found", contextTreeP->value.s, url));
      *detailsPP = (char*) "context not found";
      return false;
    }

    return orionldUserContextKeyValuesCheck(deferredContextP->tree, deferredContextP->url, detailsPP);
  }
  else if (contextTreeP->type == KjArray)
  {
    for (KjNode* itemP = contextTreeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      if (orionldUserContextKeyValuesCheck(itemP, itemP->name, detailsPP) == false)
        return false;
    }
  }
  else  // Invalid JSON type of "@context" member (Not Object, Array, nor String)
  {
    LM_E(("Invalid JSON type of '@context' member"));
    *detailsPP = (char*) "Invalid JSON type of '@context' member";
    return false;
  }

  return true;
}
