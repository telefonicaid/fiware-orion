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
#include "logMsg/logMsg.h"                                   // LM_*
#include "logMsg/traceLevels.h"                              // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                    // KjNode
#include "kjson/kjParse.h"                                   // kjParse
#include "kjson/kjFree.h"                                    // kjFree
}

#include "rest/ConnectionInfo.h"                             // ConnectionInfo
#include "orionld/common/SCOMPARE.h"                         // SCOMPAREx
#include "orionld/common/orionldErrorResponse.h"             // orionldErrorResponse
#include "orionld/common/urlParse.h"                         // urlParse
#include "orionld/context/OrionldContext.h"                  // OrionldContext
#include "orionld/context/orionldContextList.h"              // orionldContextHead, orionldContextTail
#include "orionld/context/orionldContextLookup.h"            // orionldContextLookup
#include "orionld/context/orionldContextDownloadAndParse.h"  // orionldContextDownloadAndParse

#include "orionld/context/orionldContextAdd.h"               // Own interface



// ----------------------------------------------------------------------------
//
// orionldContextAppend -
//
static bool orionldContextAppend(const char* url, KjNode* tree, char** detailsPP)
{
  OrionldContext* contextP = (OrionldContext*) malloc(sizeof(OrionldContext));

  if (contextP == NULL)
  {
    *detailsPP = (char*) "out of memory";
    return false;
  }
  
  contextP->url  = strdup(url);
  contextP->tree = tree;
  contextP->next = NULL;


  //
  // Appending new context to the list
  //
  if (orionldContextHead == NULL)
  {
    orionldContextHead = contextP;
    orionldContextTail = contextP;
  }
  else
    orionldContextTail->next = contextP;

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldContextAdd - download, parse and add a context (or various contexts) 
//
// After downloading the initial URL, the content of these "Context URLs" may be:
//
// 1. An object with a single member '@context' that is an object containing key-values:
//    "@context" {
//      "Property": "http://...",
//      "XXX";      ""
//    }
//
// 2. An object with a single member '@context', that is a vector of URL strings (https://fiware.github.io/NGSI-LD_Tests/ldContext/testFullContext.jsonld):
//    {
//      "@context": [
//        "http://...",
//        "http://...",
//        "http://..."
//      }
//    }
bool orionldContextAdd(ConnectionInfo* ciP, const char* url, char** detailsPP)
{
  LM_T(LmtContext, ("********************* Getting URL '%s' and adding it as a context", url));
  LM_T(LmtContext, ("But first, looking up '%s'", url));

  if (orionldContextLookup(url) != NULL)
  {
    LM_T(LmtContext, ("Context '%s' already cached", url));
    return true;
  }

  LM_T(LmtContext, ("Downloading and parsing URL %s", url));
  KjNode* tree = orionldContextDownloadAndParse(ciP->kjsonP, url, detailsPP);

  if (tree == NULL)
  {
    // *detailsPP taken care of by orionldContextDownloadAndParse()
    LM_E(("orionldContextDownloadAndParse returned NULL"));
    return false;
  }
  LM_T(LmtContext, ("tree is OK"));

  //
  // The resulting payload of downloading and parsing a context URL must be
  // a JSON Object with one single field, called '@context'.
  // This @context field can either be a JSON object, or a JSON Array
  // If JSON Object, the contents of the object must be a list of key-values
  // If JSON Array,  the contents of the array must be URL strings naming new contexts
  //
  // If it is an object, the list of key-values is the context and the URL is the 'name' of the context.
  // If it is an array, the array itself (naming X contexts) is the context and the the URL is the 'name' of this "complex" context.
  //

  //
  // Check that:
  //   1. the resulting payload is a JSON object
  //   2. with a single member,
  //   3. called '@context'
  //   4. that is either a JSON Object or a JSON Array
  //

  // 1. Is the resulting payload a JSON object?
  if (tree->type != KjObject)
  {
    *detailsPP = (char*) "Invalid JSON type of payload for a context - must be a JSON Object";
    return false;
  }
  LM_T(LmtContext, ("the JSON type of the tree is Object - OK"));

  // 2. Does it have one single member?
  KjNode* contextP = tree->children;
  if (contextP == NULL)
  {
    *detailsPP = (char*) "Invalid payload for a context - the payload is empty";
    return false;
  }
  LM_T(LmtContext, ("The tree has at least one child - OK"));
  
  if (contextP->next != NULL)
  {
    *detailsPP = (char*) "Invalid payload for a context - only one toplevel member allowed for contexts";
    return false;
  }
  LM_T(LmtContext, ("The tree has exactly one child - OK"));
  LM_T(LmtContext, ("Only member is named '%s' and is of type %s", contextP->name, kjValueType(contextP->type)));

  // 3. Is the single member called '@context' ?
  if (SCOMPARE9(contextP->name, '@', 'c', 'o', 'n', 't', 'e', 'x', 't', 0))
  {
    if ((contextP->type != KjObject) && (contextP->type != KjArray))
    {
      *detailsPP = (char*) "Invalid JSON type for the @context member - must be a JSON Object or a JSON Array";
      return false;
    }
  }
  else
  {
    *detailsPP = (char*) "Invalid payload for a context - the member '@context' not present";
    return false;
  }


  //
  // Creating a context for the initial URL
  //
  // This context can be either:
  // - an object with key-values (a "leaf")
  // - a vector of contexts (URL strings)
  //
  
  if (orionldContextAppend(url, tree, detailsPP) == false)
    return false;

  //
  // Now, if the context was an Object, then we are done,
  // But, if an Array, then we need to go further
  //

  // 4. Either an Object or an Array
  if (contextP->type == KjObject)
  {
    LM_T(LmtContext, ("*********************************** Was an object - we are done here"));
    return true;
  }
  else if (contextP->type != KjArray)
  {
    *detailsPP = (char*) "the '@context' field must be either a JDON Objevt or a JSON Array";
    return false;
  }

  
  LM_T(LmtContext, ("Context is an array of strings (URLs) - download and create new contexts"));

  // All items in the vector must be strings
  for (KjNode* contextItemP = contextP->children; contextItemP != NULL; contextItemP = contextItemP->next)
  {
    LM_T(LmtContext, ("URL in context array: %s", contextItemP->value.s));
    if (contextItemP->type != KjString)
    {
      *detailsPP = (char*) "Non-string found in context vector";
      LM_T(LmtContext, (*detailsPP));
      return false;
    }

    char  protocol[128];
    char  ip[256];
    short port;
    char* urlPath;
    
    if (urlParse(contextItemP->value.s, protocol, sizeof(protocol), ip, sizeof(ip), &port, &urlPath, detailsPP) == false)
    {
      LM_E(("urlParse(%s): %s", contextItemP->value.s, *detailsPP));
      *detailsPP = (char*) "invalid URL in context vector";  // overwriting the detailsPP from urlParse
      LM_T(LmtContext, (*detailsPP));
      return false;
    }
  }

  // Now go over the vector
  for (KjNode* contextItemP = contextP->children; contextItemP != NULL; contextItemP = contextItemP->next)
  {
    char* url = contextItemP->value.s;
        
    LM_T(LmtContext, ("Context is a string - meaning a new URL - download and create context: %s", url));

    tree = orionldContextDownloadAndParse(ciP->kjsonP, url, detailsPP);
    if (tree == NULL)
    {
      LM_T(LmtContext, ("orionldContextDownloadAndParse failed: %s", *detailsPP));
      return false;
    }
    if (orionldContextAppend(url, tree, detailsPP) == false)
    {
      LM_T(LmtContext, (*detailsPP));
      return false;
    }
  }
      
  return true;
}
