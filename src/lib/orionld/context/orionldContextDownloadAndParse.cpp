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
#include "kjson/kjson.h"                                    // Kjson
#include "kjson/KjNode.h"                                   // KjNode
#include "kjson/kjParse.h"                                  // kjParse
#include "kjson/kjFree.h"                                   // kjFree
}

#include "orionld/common/OrionldResponseBuffer.h"              // OrionldResponseBuffer
#include "orionld/common/orionldRequestSend.h"                 // orionldRequestSend
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextDownloadAndParse.h"    // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextDownloadAndParse -
//
// The returned tree must be freed by the caller
//
KjNode* orionldContextDownloadAndParse(Kjson* kjsonP, const char* url, char** detailsPP)
{
  //
  // Prepare the httpResponse buffer
  //
  // A smarter way here would be to use a thread local buffer and allocate only if that buffer
  // is not big enough
  //
  OrionldResponseBuffer  httpResponse;
  
  httpResponse.buf       = (char*) malloc(1024);
  httpResponse.size      = 1024;
  httpResponse.used      = 0;
  httpResponse.allocated = true;

  if (httpResponse.buf == NULL)
  {
    *detailsPP = (char*) "out of memory";
    return NULL;
  }

  LM_T(LmtContext, ("Downloading context '%s'", url));
  if (orionldRequestSend(&httpResponse, url, 5000, detailsPP) == false)
  {
    //
    // detailsPP is filled in by orionldRequestSend()
    // httpResponse.buf freed by orionldRequestSend() in case of error
    //
    LM_E(("orionldRequestSend failed: %s", *detailsPP));
    return NULL;
  }
  LM_TMP(("Got @context - parsing it"));

  // Now parse the payload
  KjNode* tree = kjParse(kjsonP, httpResponse.buf);
  LM_TMP(("Got @context - parsed it"));

  // And then we can free the httpResponse buffer
  free(httpResponse.buf);

  if (tree == NULL)
  {
    *detailsPP = (char*) "Error parsing context";
    LM_E(("kjParse returned NULL"));
    return NULL;
  }

  if ((tree->type != KjArray) && (tree->type != KjString) && (tree->type != KjObject))
  {
    kjFree(tree);
    *detailsPP = (char*) "Invalid JSON type of response";
    LM_E((*detailsPP));
    return NULL;
  }

  
  //
  // Lastly, make sure the context is not shadowing any alias form the Core Context
  // If the Core Context is NULL, then this check is NOT done, as we are processing the
  // Core Context ...
  //
  // A @context can be:
  //   1. A string, referencing another context
  //   2. A vector of strings, referencing a number of contexts
  //   3. An object, with one member named '@context', that is an object and contains a number of key-values (alias-value)
  //
  // If the current context if an object, then we need to make sure that no alias is shadowing any of the Core Context aliases
  //
  // FIXME: This loop is VERY VERY SLOW.
  //        Many many strcmp are performed each time a new context is introduced.
  //        By keeping the checksum of the names in de default context and calculating the checksums of the names
  //        in the new checksum, 99.9% of the strcmp's can be replaced by simple integer comparisons.
  //
  if (orionldCoreContext.tree == NULL)
  {
    LM_TMP(("This is the Core Context - we are done here"));
    return tree;
  }
  
  if (tree->type != KjObject)
  {
    kjFree(tree);
    *detailsPP = (char*) "Not a JSON Object - invalid @context";
    LM_E((*detailsPP));
    return NULL;
  }

  KjNode* contextNodeP = tree->children;
  LM_TMP(("contextNodeP is named '%s'", contextNodeP->name));

  if (contextNodeP == NULL)
  {
    kjFree(tree);
    *detailsPP = (char*) "Invalid context - object must have a single member, called '@context'";
    LM_E((*detailsPP));
    return NULL;
  }

  if (strcmp(contextNodeP->name, "@context") != 0)
  {
    kjFree(tree);
    *detailsPP = (char*) "Invalid context - object must have a single member, called '@context'";
    LM_E((*detailsPP));
    return NULL;
  }

  if (contextNodeP->children == NULL)
  {
    kjFree(tree);
    *detailsPP = (char*) "Invalid context - '@context' is empty";
    LM_E((*detailsPP));
    return NULL;
  }

  if (contextNodeP->next != NULL)
  {
    kjFree(tree);
    *detailsPP = (char*) "Invalid context - '@context' must be the only member of the JSON object";
    LM_E((*detailsPP));
    return NULL;
  }

  // Now, we have '@context' - is it an object?
  if (contextNodeP->type != KjObject)
  {
    LM_TMP(("Not an object ('@context' in '%s' is of type '%s') - we are done here (no collision check necessary)", url, kjValueType(contextNodeP->type)));
    return tree;
  }

  LM_TMP(("Starting collision loop"));
  for (KjNode* kNodeP = contextNodeP->children; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    LM_TMP(("Checking collision for '%s'", kNodeP->name));

    for (KjNode* coreNodeP = orionldCoreContext.tree->children; coreNodeP != NULL; coreNodeP = coreNodeP->next)
    {
      LM_TMP(("Comparing '%s' to '%s'", kNodeP->name, coreNodeP->name));
      if (strcmp(kNodeP->name, coreNodeP->name) == 0)
      {
        LM_E(("New context collides with core context. Offending alias: '%s'", kNodeP->name));
        kjFree(tree);
        *detailsPP = (char*) "Invalid context - colliding with Core Context";
        LM_E((*detailsPP));
        return NULL;
      }
    }
  }

  return tree;
}
