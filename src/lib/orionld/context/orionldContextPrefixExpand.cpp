/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "kalloc/kaAlloc.h"                                    // kaAlloc
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // kalloc
#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldUriExpand.h"                  // orionldUriExpand
#include "orionld/context/orionldContextPrefixExpand.h"        // Own interface



__thread char     cachedPrefixName[128];
__thread char     cachedPrefixValueV[256];
__thread char*    cachedPrefixValueP = NULL;
__thread int      cachedPrefixValueLen;



// -----------------------------------------------------------------------------
//
// prefixExpand -
//
// FIXME: the kaAlloc instance to use (global or thread allocation) should be a parameter to this function
//
static void prefixExpand(OrionldContext* contextP, KjNode* kNodeP)
{
  char* colonP;
  char* rest;

  if ((colonP = strchr(kNodeP->value.s, ':')) == NULL)
    return;
  if (colonP[1] == '/')
    return;

  // colon found, need to replace
  int   sLen;
  bool  keepShortName = false;
  char* alias         = kNodeP->value.s;

  *colonP = 0;           // By NULLing the ':', 'alias' now is the prefix
  rest    = &colonP[1];  // rest points to what must be appended after the value of the prefix

  // LM_TMP(("KZ: Trying to expand '%s' in %s:%s of context '%s'", alias, alias, rest, contextP->url));
  // If prefix not already looked up, look it up
  if ((cachedPrefixValueP == NULL) || (strcmp(alias, cachedPrefixName) != 0))
  {
    char* details;

    if (orionldUriExpand(contextP, alias, cachedPrefixValueV, sizeof(cachedPrefixValueV), &details) == true)
    {
      strcpy(cachedPrefixName, alias);
      cachedPrefixValueP    = cachedPrefixValueV;
      cachedPrefixValueLen  = strlen(cachedPrefixValueV);
    }
    else
      keepShortName = true;
  }

  if (keepShortName == true)
  {
    // No match - re-establish the destroyed ':' and do no more - we keep the original name "xxx:yyy"
    *colonP = ':';
    // LM_TMP(("Keeping short name '%s'", kNodeP->value.s));
  }
  else
  {
    sLen = strlen(rest) + cachedPrefixValueLen;
    kNodeP->value.s = kaAlloc(&kalloc, sLen + 1);
    snprintf(kNodeP->value.s, sLen, "%s%s", cachedPrefixValueP, rest);
    // LM_TMP(("KZ: Expanded value to '%s' (cachedPrefixValueP: '%s', rest: '%s')", kNodeP->value.s, rest));
  }
}



// -----------------------------------------------------------------------------
//
// orionldContextPrefixExpand -
//
// The value of 'contextP' points to the linked list of key-values of the @context.
// This function goes through all key-values and looks up/replaces any prefixes.
//
// It is the responsibility of the caller to make sure that the context 'contextP' refers to
// a context of this type:
//
//   { "@context": { "key": "value, "key": "value", "key": { "@id": "", "@type": "" }, ... } }
//
//
// Now, what is to be expanded are the values of the context. E.g.:
//
// {
//   "@context": {
//     "fiware":   "https://uri.fiware.org/ns/datamodels#",
//     "tutorial": "https://fiware.github.io/tutorials.Step-by-Step/schema/",
//     "xsd":      "http://www.w3.org/2001/XMLSchema#",

//     ...,
//     "description": "fiware:description",
//     "maxCapacity":  {
//       "@id": "tutorial:maxCapacity",
//       "@type": "xsd:integer",
//     }
//   }
// }
//
// Expansions for this example:
//   1. "fiware:description"   -> "https://uri.fiware.org/ns/datamodels#description"
//   2. "tutorial:maxCapacity" -> "https://fiware.github.io/tutorials.Step-by-Step/schema/maxCapacity"
//   3. "xsd:integer"          -> "http://www.w3.org/2001/XMLSchema#integer"
//
// FIXME: the kaAlloc instance to use (global or thread allocation) should be a parameter to this function
//
void orionldContextPrefixExpand(OrionldContext* contextP)
{
  KjNode*  tree = contextP->tree;

  // LM_TMP(("KZ: In orionldContextPrefixExpand for context '%s'", contextP->url));

  cachedPrefixValueP = NULL;  // Clear "cache" before lookup starts

  for (KjNode* kNodeP = tree->value.firstChildP->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (kNodeP->type == KjString)
      prefixExpand(contextP, kNodeP);
    else if (kNodeP->type == KjObject)
    {
      for (KjNode* objectNodeP = kNodeP->value.firstChildP; objectNodeP != NULL; objectNodeP = objectNodeP->next)
      {
        if (objectNodeP->type == KjString)
          prefixExpand(contextP, objectNodeP);
      }
    }
  }
}
