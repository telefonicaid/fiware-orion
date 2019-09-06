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

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/rest.h"                                         // restPortGet
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/rest/orionldServiceInit.h"                   // orionldHostNameLen
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextCreateFromUrl.h"       // orionldContextCreateFromUrl
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextAdd.h"                 // orionldContextAdd
#include "orionld/context/orionldContextInlineCheck.h"         // orionldContextInlineCheck
#include "orionld/context/orionldContextPrefixExpand.h"        // orionldContextPrefixExpand
#include "orionld/context/orionldContextTreat.h"               // Own interface



// ----------------------------------------------------------------------------
//
// contextItemNodeTreat -
//
static OrionldContext* contextItemNodeTreat(ConnectionInfo* ciP, char* url)
{
  char*            details;
  OrionldContext*  contextP = orionldContextAdd(ciP, url, OrionldUserContext, &details);

  if (contextP == NULL)
  {
    LM_E(("Invalid context '%s': %s", url, details));
    orionldState.contextP = NULL;  // Leak?
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid context", details, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return NULL;
  }

  return contextP;
}



// -----------------------------------------------------------------------------
//
// orionldContextTreat -
//
//
// The allowed payloads for the @context member are:
//
// 1. ARRAY - An array of URL strings:
//    "@context": [
//      "http://...",
//      "http://...",
//      "http://..."
//    }
//
// 2. STRING - A single URL string:
//    "@context": "http://..."
//
// 3. OBJECT - Direct context:
//    "@context": {
//      "Property": "http://...",
//      "XXX";      "YYY"
//    }
//
// -----------------------------------------------------------
// Case 3 is not implemented in the first round. coming later.
// -----------------------------------------------------------
//
// As the payload is already parsed, what needs to be done here is to call orionldContextAdd() for each of these URLs
//
// The content of these "Context URLs" can be:
//
// 4. An object with a single member '@context' that is an object containing key-values:
//    "@context" {
//      "Property": "http://...",
//      "XXX";      ""
//    }
//
// 5. An object with a single member '@context', that is a vector of URL strings (https://fiware.github.io/NGSI-LD_Tests/ldContext/testFullContext.jsonld):
//    {
//      "@context": [
//        "http://...",
//        "http://...",
//        "http://..."
//      }
//    }
//
bool orionldContextTreat
(
  ConnectionInfo*     ciP,
  KjNode*             contextNodeP
)
{
  if (contextNodeP->type == KjString)
  {
    char* details;

    if ((orionldState.contextP = orionldContextCreateFromUrl(ciP, contextNodeP->value.s, OrionldUserContext, &details)) == NULL)
    {
      LM_E(("Failed to create context from URL: %s", details));
      orionldErrorResponseCreate(OrionldBadRequestData, "Failure to create context from URL", details, OrionldDetailsString);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }

    orionldState.contextToBeFreed = false;                  // context has been added to public list - must not be freed
  }
  else if (contextNodeP->type == KjArray)
  {
    char* details;

    orionldState.contextP = orionldContextCreateFromTree(contextNodeP, orionldState.link, OrionldUserContext, &details);

    if (orionldState.contextP == NULL)
    {
      LM_E(("Failed to create context from Tree : %s", details));
      orionldErrorResponseCreate(OrionldBadRequestData, "Failure to create context from tree", details, OrionldDetailsString);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }

    //
    // The context containing a vector of X context strings (URLs) must be freed
    // The downloaded contexts though are added to the public list and will not be freed)
    //
    orionldState.contextToBeFreed = true;

    for (KjNode* contextArrayItemP = contextNodeP->value.firstChildP; contextArrayItemP != NULL; contextArrayItemP = contextArrayItemP->next)
    {
      if (contextArrayItemP->type == KjString)
      {
        if (contextItemNodeTreat(ciP, contextArrayItemP->value.s) == NULL)
        {
          LM_E(("contextItemNodeTreat failed"));
          // Error payload set by contextItemNodeTreat
          orionldState.contextP = NULL;  // Leak?
          return false;
        }
      }
      else if (contextArrayItemP->type == KjObject)
      {
        if (orionldContextTreat(ciP, contextArrayItemP) == false)
        {
          LM_E(("Error treating context object inside array"));
          orionldState.contextP = NULL;  // Leak?
          orionldErrorResponseCreate(OrionldBadRequestData, "Error treating context object inside array", NULL, OrionldDetailsString);
          ciP->httpStatusCode = SccBadRequest;
          return false;
        }
      }
      else
      {
        LM_E(("Context Array Item is not a String nor an Object, but of type '%s'", kjValueType(contextArrayItemP->type)));
        orionldState.contextP = NULL;  // Leak?
        orionldErrorResponseCreate(OrionldBadRequestData, "Context Array Item is of an unsupported type", NULL, OrionldDetailsString);
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }
    }
  }
  else if (contextNodeP->type == KjObject)
  {
    if (orionldContextInlineCheck(ciP, contextNodeP) == false)
    {
      // orionldContextInlineCheck sets the error response
      LM_E(("Invalid inline context"));
      orionldState.contextP = NULL;
      return false;
    }

    orionldState.inlineContext.url       = orionldState.link;
    orionldState.inlineContext.tree      = contextNodeP;
    orionldState.inlineContext.type      = OrionldUserContext;
    orionldState.inlineContext.ignore    = false;
    orionldState.inlineContext.temporary = true;
    orionldState.inlineContext.next      = NULL;
    orionldState.contextP                = &orionldState.inlineContext;

    orionldContextPrefixExpand(orionldState.contextP, true);

    return true;
  }
  else
  {
    LM_E(("invalid JSON type of @context member"));
    orionldState.contextP = NULL;  // Leak?
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid context", "invalid JSON type of @context member", OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }


#if 0
  //
  // Check the validity of the context
  //

  //
  // FIXME: task/28.orionld-user-context-cannot-use-core-context-values
  //        The check is implemented, but ... six different types of contexts ... just too much
  //        This test (orionldUserContextKeyValuesCheck) is postponed until hardening/xx.orionld-order-in-contexts
  //        is implemented.
  //
  char* details;

  if (orionldUserContextKeyValuesCheck(orionldState.contextP->tree, orionldState.contextP->url, &details) == false)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid context", details, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }
#endif

  return true;
}
