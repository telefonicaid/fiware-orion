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
#include "kjson/kjParse.h"                                     // kjParse
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponse
#include "orionld/common/urlParse.h"                           // urlParse
#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextDownloadAndParse.h"    // orionldContextDownloadAndParse
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextCreateFromUrl.h"       // orionldContextCreateFromUrl
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/context/orionldContextAppend.h"              // orionldContextAppend
#include "orionld/context/orionldContextAdd.h"                 // Own interface



// ----------------------------------------------------------------------------
//
// orionldContextAdd - download, parse and add a context (or various contexts)
//
// This function is called by orionldPostEntities to add referenced contexts.
//
// In the payload, there are three possibilities for contexts.
// Not that the payload is already parsed and what enters this function is the value of the "@context" member:
//
// 1. A Vector of contexts as strings:
//    [
//      "http://...",
//      "http://...",
//      "http://..."
//    ]
//
// 2. A String that references a context:
//    "http://..."
//
// 3. Direct context:
//    {
//      "Property": "http://...",
//      "XXX";      "YYY"
//    }
//
//
// If it is not a "Direct Context" (case 1), then one or more contexts may have to be downloaded.
// The syntax of these context payload may be of two different types:
//
// 4. An object with a single member '@context' that is an object containing key-values:
//    "@context" {
//      "Property": "http://...",
//      "XXX";      "YYY"
//    }
//
// 5. An object with a single member '@context', that is a vector of URL strings (https://fiware.github.io/NGSI-LD_Tests/ldContext/testFullContext.jsonld):
//    {
//      "@context": [
//        "http://...",
//        "http://...",
//        "http://..."
//      ]
//    }
//
// The first three cases are taken care of by orionldPostEntities (case 3 is not implemented in the first round)
//
// So, in this routine, the payload of the downloaded url (by calling orionldContextDownloadAndParse)
// must be a jSON Object with a single member "@context",
// This member is either a JSON Array or an Object.
//
//
// The resulting payload of downloading and parsing a context URL must be
// a JSON Object with one single field, called '@context'.
// This @context field can either be a JSON object, or a JSON Array
// If JSON Object, the contents of the object must be a list of key-values
// If JSON Array,  the contents of the array must be URL strings naming new contexts
//
// If it is an object, the list of key-values is the context and the URL is the 'name' of the context.
// If it is an array, the array itself (naming X contexts) is the context and the the URL is the 'name'
// of this "complex" context.
//
OrionldContext* orionldContextAdd
(
  ConnectionInfo*     ciP,
  const char*         url,
  OrionldContextType  contextType,
  char**              detailsPP
)
{
  OrionldContext* contextP = NULL;

  LM_T(LmtContext, ("********************* Getting context in URL '%s' and adding it as a context", url));
  LM_T(LmtContext, ("But first, looking up '%s'", url));

  if ((contextP = orionldContextLookup(url)) != NULL)
  {
    LM_T(LmtContext, ("Looking up context '%s': already cached", url));

    return contextP;
  }

  LM_T(LmtContext, ("Downloading and parsing context of URL '%s'", url));
  LM_TMP(("Downloading and parsing context '%s' (using global kjsonP - as the context needs to survive between requests)", url));
  KjNode* tree = orionldContextDownloadAndParse(kjsonP, url, detailsPP);

  if (tree == NULL)
  {
    // *detailsPP taken care of by orionldContextDownloadAndParse()
    LM_E(("orionldContextDownloadAndParse returned NULL"));
    return NULL;
  }
  LM_T(LmtContext, ("tree is OK"));

  //
  // Check that:
  //   1. the resulting payload is a JSON object
  //   2. with a single member,
  //   3. called '@context'
  //   4. that is either a JSON Object or a JSON Array
  //   5. Optionally, one mor emember is allowed: "generatedAt", a String
  //

  // 1. Is the resulting payload a JSON object?
  if (tree->type != KjObject)
  {
    *detailsPP = (char*) "Invalid JSON type of payload for a context - must be a JSON Object";
    return NULL;
  }
  LM_T(LmtContext, ("the JSON type of the tree is Object - OK"));

  // 2. Does it have one single member (and optionally "generatedAt")?
  if (tree->value.firstChildP == NULL)
  {
    *detailsPP = (char*) "Invalid payload for a context - the payload is empty";
    return NULL;
  }
  LM_T(LmtContext, ("The tree has at least one child - OK"));

  if (tree->value.firstChildP->next != NULL)
  {
    if ((strcmp(tree->value.firstChildP->next->name, "generatedAt") == 0) && (tree->value.firstChildP->next->type == KjString))
    {
      // OK
    }
    else
    {
      *detailsPP = (char*) "Invalid payload for a context - only members allowed for context payloads are '@context' and 'generatedAt'";
      return NULL;
    }
  }

  LM_T(LmtContext, ("The tree has one child, optionally two - OK"));
  LM_T(LmtContext, ("First member is '%s' and of type %s", tree->value.firstChildP->name, kjValueType(tree->value.firstChildP->type)));

  // 3. Is the single member called '@context' ?
  if (!SCOMPARE9(tree->value.firstChildP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
  {
    *detailsPP = (char*) "Invalid payload for a context - the member '@context' not present";
    return NULL;
  }

  // 4. Is it a JSON Object or Array?
  if ((tree->value.firstChildP->type != KjObject) && (tree->value.firstChildP->type != KjArray))
  {
    *detailsPP = (char*) "Invalid JSON type for the @context member - must be a JSON Object or a JSON Array";
    return NULL;
  }


  //
  // Creating a context for the initial URL
  //
  // This context can be either:
  // - an object with key-values (a "leaf")
  // - a vector of contexts (URL strings)
  //

  if ((contextP = orionldContextAppend(url, tree, contextType, detailsPP)) == NULL)
    return NULL;

  //
  // Now, if the context was an Object, then we are done,
  // But, if an Array, then we need to go further
  //

  // 4. Either an Object or an Array
  if (contextP->tree->type == KjObject)
  {
    LM_T(LmtContext, ("*********************************** Was an object - we are done here"));
    return contextP;
  }


  LM_T(LmtContext, ("Context is an array of strings (URLs) - download and create new contexts"));

  //
  // All items in the vector must be strings, containing syntactically correct URLs
  //
  for (KjNode* contextItemP = contextP->tree->value.firstChildP; contextItemP != NULL; contextItemP = contextItemP->next)
  {
    LM_T(LmtContext, ("URL in context array: %s", contextItemP->value.s));
    if (contextItemP->type != KjString)
    {
      *detailsPP = (char*) "Non-string found in context vector";
      LM_T(LmtContext, (*detailsPP));

      free(contextP->url);
      free(contextP);

      return NULL;
    }

    char      protocol[128];
    char      ip[256];
    uint16_t  port;
    char*     urlPath;

    if (urlParse(contextItemP->value.s, protocol, sizeof(protocol), ip, sizeof(ip), &port, &urlPath, detailsPP) == false)
    {
      LM_E(("urlParse(%s): %s", contextItemP->value.s, *detailsPP));
      *detailsPP = (char*) "invalid URL in context vector";  // overwriting the detailsPP from urlParse
      LM_T(LmtContext, (*detailsPP));

      free(contextP->url);
      free(contextP);

      return NULL;
    }
  }

  // Now go over the vector
  for (KjNode* contextItemP = contextP->tree->value.firstChildP; contextItemP != NULL; contextItemP = contextItemP->next)
  {
    char* url = contextItemP->value.s;

    LM_T(LmtContext, ("Context is a string - meaning a new URL - download and create context: %s", url));

    // If already in "cache", no need to download and parse
    if (orionldContextLookup(url) != NULL)
      continue;

    tree = orionldContextDownloadAndParse(orionldState.kjsonP, url, detailsPP);
    if (tree == NULL)
    {
      LM_T(LmtContext, ("orionldContextDownloadAndParse failed: %s", *detailsPP));

      free(contextP->url);
      free(contextP);

      return NULL;
    }

    // FIXME: tree = kjClone(tree); ?
    if (orionldContextAppend(url, tree, contextType, detailsPP) == NULL)
    {
      LM_T(LmtContext, (*detailsPP));

      free(contextP->url);
      free(contextP);

      return NULL;
    }
  }

  return contextP;
}
