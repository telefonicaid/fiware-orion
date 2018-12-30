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
#include "orionld/common/SCOMPARE.h"                           // SCOMPARE
#include "orionld/rest/orionldServiceInit.h"                   // orionldHostNameLen
#include "orionld/context/orionldContextItemLookup.h"          // orionldContextItemLookup
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldUriExpand.h"                  // Own interface



// ----------------------------------------------------------------------------
//
// uriExpansion -
//
// 1. Lookup in user context, expand if found
// 2. Lookup in Core Context - if found there, no expansion is made, just return from function
// 3. Expand using the default context
//
// Returns the number of expansions done.
//  -2: expansion NOT found in the context - use default URL
//  -1: an error has occurred
//   0: found in Core context only. No expansion is to be made
//   1: expansion found only for the attr-name/entity-type
//   2: expansions found both for attr-name and attr-type
//
int uriExpansion(OrionldContext* contextP, const char* name, char** expandedNameP, char** expandedTypeP, char** detailsPP)
{
  KjNode*      contextValueP = NULL;
  const char*  contextUrl    = (contextP == NULL)? "NULL" : contextP->url;  // For debugging only

  *expandedNameP = NULL;
  *expandedTypeP = NULL;

  LM_T(LmtUriExpansion, ("expanding '%s'", name));

  //
  // Use the default context?
  // Two possibilities for default context:
  // 1. No user context present
  // 2. Not found in user context
  //
  // But, if found in Core Context, then NO Expansion is to be made
  //
  if (contextP != NULL)
  {
    LM_T(LmtUriExpansion, ("looking up context item for '%s', in context '%s'", name, contextUrl));
    contextValueP = orionldContextItemLookup(contextP, name);
  }

  if (contextValueP == NULL)
  {
    LM_T(LmtUriExpansion, ("'%s' was not found in context '%s'", name, contextUrl));
    //
    // Not found.
    // Now, if found in Core context, no expansion is to be made (return 0).
    // If not, then the default URL should be used (return 3).
    //
    contextValueP = orionldContextItemLookup(&orionldCoreContext, name);

    if (contextValueP != NULL)
    {
      LM_T(LmtUriExpansion, ("'%s' found in Core Context - no expansion to be made", name));
      return 0;
    }

    LM_T(LmtUriExpansion, ("'%s' NOT found in Core Context - DEFAULT expansion to be made", name));
    return -2;
  }

  LM_T(LmtUriExpansion, ("Found '%s', in context '%s'", name, contextUrl));

  //
  // Context Item found - must be either a string or an object containing two strings
  //
  LM_T(LmtUriExpansion, ("contextValueP at %p", contextValueP));
  if (contextValueP->type == KjString)
  {
    LM_T(LmtUriExpansion, ("got a string - expanded name for '%s' is '%s'", name, contextValueP->value.s));
    *expandedNameP = contextValueP->value.s;

    return 1;
  }

  if (contextValueP->type != KjObject)
  {
    // FIXME: I need ciP here to fill in the error-response
    return -1;
  }

  //
  // The context item has a complex value: "@id" and "@type".
  // The value of:
  // - "@id":     corresponds to the 'name' of the attribute
  // - "@type":   corresponds to the 'type' of the attribute??? FIXME
  //
  int children = 0;
  for (KjNode* nodeP = contextValueP->children; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE4(nodeP->name, '@', 'i', 'd', 0))
    {
      *expandedNameP = nodeP->value.s;
      LM_T(LmtUriExpansion, ("got an object - expanded name is '%s'", nodeP->value.s));
    }
    else if (SCOMPARE6(nodeP->name, '@', 't', 'y', 'p', 'e', 0))
    {
      *expandedTypeP = nodeP->value.s;
      LM_T(LmtUriExpansion, ("got an object - expanded type is '%s'", nodeP->value.s));
    }
    else
    {
      *detailsPP = (char*) "Invalid context - invalid field in context item object";
      LM_E(("uriExpansion: Invalid context - invalid field in context item object: '%s'", nodeP->name));
      return -1;
    }
    ++children;
  }

  //
  // If an expansion has been found, we MUST have a "@id", if not - ERROR
  //
  if ((children >= 1) && (*expandedNameP == NULL))
  {
    *detailsPP = (char*) "Invalid context - no @id in complex context item";
    return -1;
  }


  //
  // FIXME: Is this assumption true?
  //        If the value of a @context item is an object, must it have exactly TWO members, @id and @type?
  //
  // FIXME: This check should NOT be done here, every time, but ONCE when the context is first downloaded
  //
  if ((children != 2) || (*expandedNameP == NULL) || (*expandedTypeP == NULL))
  {
    *detailsPP = (char*) "Invalid context - field in context item object not matching the rules";
    LM_E(("uriExpansion: invalid @context item '%s' in @context '%s'", name, contextUrl));
    return -1;
  }

  LM_T(LmtUriExpansion, ("returning %d (expansions found): name='%s', type='%s'", children, *expandedNameP, *expandedTypeP));
  return children;
}



// -----------------------------------------------------------------------------
//
// orionldUriExpand -
//
bool orionldUriExpand(OrionldContext* contextP, char* shortName, char* longName, int longNameLen, char** detailsP)
{
  char* expandedName;
  char* expandedType;
  int   n;

  n = uriExpansion(contextP, shortName, &expandedName, &expandedType, detailsP);
  if (n == -1)
  {
    LM_E(("uriExpansion error: %s", *detailsP));
    return false;
  }
  else if (n == -2)  // expansion NOT found in the context - use default URL
  {
    //
    // FIXME:
    //   What if it is not a shortname in URI param?
    //   Check for http:// in typeName?
    //
    snprintf(longName, longNameLen, "%s%s", orionldDefaultUrl, shortName);
  }
  else  // expansion found
  {
    snprintf(longName, longNameLen, "%s", expandedName);
  }

  LM_T(LmtUriExpansion, ("KZ: expanded '%s' to '%s'", shortName, longName));
  return true;
}
