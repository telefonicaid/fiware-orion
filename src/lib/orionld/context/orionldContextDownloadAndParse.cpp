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
}

#include "orionld/common/OrionldResponseBuffer.h"              // OrionldResponseBuffer
#include "orionld/common/orionldRequestSend.h"                 // orionldRequestSend
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextDownloadAndParse.h"    // Own interface



void contextArrayPresent(KjNode* tree, const char* what)
{
  if (tree == NULL)
    return;

  if ((tree->type == KjObject) && (tree->value.firstChildP->type == KjArray))
  {
    int childNo = 0;

    LM_T(LmtContext, ("Got a @context array (%s):", what));
    for (KjNode* kP = tree->value.firstChildP->value.firstChildP; kP != NULL; kP = kP->next)
    {
      LM_T(LmtContext, ("  Child %d: %s (at %p)", childNo, kP->value.s, kP));
      ++childNo;
    }
  }
}


static __thread OrionldResponseBuffer  httpResponse;
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
  bool ok = false;

  for (int tries = 0; tries < 5; tries++)
  {
    httpResponse.buf       = httpResponse.internalBuffer;  // Contexts to be saved will need to be cloned
    httpResponse.size      = sizeof(httpResponse.internalBuffer);
    httpResponse.used      = 0;
    httpResponse.allocated = true;

    if (httpResponse.buf == NULL)
    {
      *detailsPP = (char*) "out of memory";
      return NULL;
    }

    LM_T(LmtContext, ("Downloading context '%s'", url));

    //
    // detailsPP is filled in by orionldRequestSend()
    // httpResponse.buf freed by orionldRequestSend() in case of error
    //
    bool tryAgain = false;
    if (orionldRequestSend(&httpResponse, url, 10000, detailsPP, &tryAgain) == true)
    {
      ok = true;
      break;
    }
    else
      LM_E(("orionldRequestSend failed (try number %d out of 5): %s", tries + 1, *detailsPP));

    if (tryAgain == false)
      break;
  }

  if (ok == false)
  {
    LM_E(("orionldRequestSend failed"));
    // detailsPP filled in by orionldRequestSend
    return NULL;
  }

  // Now parse the payload
  // LM_T(LmtContext, ("Got @context: %s", httpResponse.buf));
  // LM_T(LmtContext, ("Got @context - parsing it"));
  char tmpBuf[128];
  strncpy(tmpBuf, httpResponse.buf, 128);
  tmpBuf[127] = 0;
  LM_TMP(("Got @context: %s", tmpBuf));
  KjNode* tree = kjParse(kjsonP, httpResponse.buf);
  LM_T(LmtContext, ("Got @context - parsed it"));

  // <DEBUG>
  extern void contextArrayPresent(KjNode* tree, const char* what);
  contextArrayPresent(tree, "Just after parsing");
  // </DEBUG>

  if (tree == NULL)
  {
    *detailsPP = kjsonP->errorString;
    LM_E(("kjParse returned NULL"));
    return NULL;
  }

  // <DEBUG>
  extern void contextArrayPresent(KjNode* tree, const char* what);
  contextArrayPresent(tree, "Just after freeing httpResponse");
  // </DEBUG>
  
  if ((tree->type != KjArray) && (tree->type != KjString) && (tree->type != KjObject))
  {
    LM_T(LmtContext, ("Freeing tree as wrong json type"));
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
    LM_T(LmtContext, ("This is the Core Context - we are done here"));
    return tree;
  }
  
  if (tree->type != KjObject)
  {
    LM_T(LmtContext, ("tree->type != KjObject : freeing context tree"));
    *detailsPP = (char*) "Not a JSON Object - invalid @context";
    LM_E((*detailsPP));
    return NULL;
  }

  KjNode* contextNodeP = tree->value.firstChildP;
  LM_T(LmtContext, ("contextNodeP is named '%s'", contextNodeP->name));

  if (contextNodeP == NULL)
  {
    *detailsPP = (char*) "Invalid context - object must have a single member, called '@context'";
    LM_E((*detailsPP));
    return NULL;
  }

  if (strcmp(contextNodeP->name, "@context") != 0)
  {
    *detailsPP = (char*) "Invalid context - object must have a single member, called '@context'";
    LM_E((*detailsPP));
    return NULL;
  }

  if (contextNodeP->value.firstChildP == NULL)
  {
    *detailsPP = (char*) "Invalid context - '@context' is empty";
    LM_E((*detailsPP));
    return NULL;
  }

  if (contextNodeP->next != NULL)
  {
    if ((strcmp(contextNodeP->next->name, "generatedAt") == 0) && (contextNodeP->next->type == KjString))
    {
      // OK
    }
    else
    {
      *detailsPP = (char*) "Invalid context - '@context' and optionally 'generatedAt' must be the only members of the JSON object";
      LM_E((*detailsPP));
      return NULL;
    }
  }

  // Now, we have '@context' - is it an object?
  if (contextNodeP->type != KjObject)
  {
    LM_T(LmtContext,  ("Not an object ('@context' in '%s' is of type '%s') - we are done here (no collision check necessary)",
                       url, kjValueType(contextNodeP->type)));

    // <DEBUG>
    if ((tree->type == KjObject) && (tree->value.firstChildP->type == KjArray))
    {
      int childNo = 0;

      LM_T(LmtContext, ("Just before returning the tree: got a @context array:"));
      for (KjNode* kP = tree->value.firstChildP->value.firstChildP; kP != NULL; kP = kP->next)
      {
        LM_T(LmtContext, ("  Child %d: %s", childNo, kP->value.s));
        ++childNo;
      }
    }
    // </DEBUG>

    return tree;
  }

  LM_T(LmtContext, ("Starting collision loop"));
  for (KjNode* kNodeP = contextNodeP->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    LM_T(LmtContext, ("Checking for collisions for context '%s'", kNodeP->name));

    for (KjNode* coreNodeP = orionldCoreContext.tree->value.firstChildP; coreNodeP != NULL; coreNodeP = coreNodeP->next)
    {
      // LM_T(LmtContext, ("Comparing '%s' to '%s'", kNodeP->name, coreNodeP->name));
      if (strcmp(kNodeP->name, coreNodeP->name) == 0)
      {
        LM_E(("New context collides with core context. Offending alias: '%s'", kNodeP->name));
        *detailsPP = (char*) "Invalid context - colliding with Core Context";
        LM_E((*detailsPP));
        return NULL;
      }
    }
  }

  return tree;
}
