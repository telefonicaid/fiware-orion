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
#include "ngsi/ContextAttribute.h"                             // ContextAttribute
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/rest/orionldServiceInit.h"                   // orionldHostNameLen
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextCreateFromUrl.h"       // orionldContextCreateFromUrl
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextAdd.h"                 // orionldContextAdd
#include "orionld/context/orionldContextInlineCheck.h"         // orionldContextInlineCheck
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
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", details, OrionldDetailsString);
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
  KjNode*             contextNodeP,
  char*               entityId,
  ContextAttribute**  caPP
)
{
  if (contextNodeP->type == KjString)
  {
    char* details;

    LM_TMP(("KZ: Setting orionldState.contextP to the output from orionldContextCreateFromUrl"));
    if ((orionldState.contextP = orionldContextCreateFromUrl(ciP, contextNodeP->value.s, OrionldUserContext, &details)) == NULL)
    {
      LM_E(("Failed to create context from URL: %s", details));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from URL", details, OrionldDetailsString);
      return false;
    }

    orionldState.contextToBeFreed = false;                  // context has been added to public list - must not be freed
  }
  else if (contextNodeP->type == KjArray)
  {
    char* details;

    //
    // REMEMBER
    //   This context is just the array of context-strings: [ "url1", "url2" ]
    //   The individual contexts (the items of the vector - "url1", ...) are treated a few lines down
    //
    // contextUrl = "http://" host + ":" + port + "/ngsi-ld/ex/v1/contexts/" + entity.id;
    //
    unsigned int  linkPathLen = 7 + orionldHostNameLen + 1 + 5 + 27 + strlen(entityId) + 1;
    char          linkPathV[256];
    char*         linkPath;

    if (linkPathLen > sizeof(linkPathV))
    {
      linkPath = (char*) malloc(linkPathLen);
      if (linkPath == NULL)
      {
        LM_E(("out of memory creating Link HTTP Header"));
        orionldErrorResponseCreate(ciP, OrionldInternalError, "cannot create Link HTTP Header", "out of memory", OrionldDetailsString);
        return false;
      }
    }
    else
      linkPath = linkPathV;

    snprintf(linkPath, linkPathLen, "http://%s:%d/ngsi-ld/ex/v1/contexts/%s", orionldHostName, restPortGet(), entityId);

    LM_TMP(("KZ: Setting orionldState.contextP to the output from orionldContextCreateFromTree"));
    orionldState.contextP = orionldContextCreateFromTree(contextNodeP, linkPath, OrionldUserContext, &details);

    if (linkPath != linkPathV)
      free(linkPath);  // orionldContextCreateFromTree strdups the URL

    if (orionldState.contextP == NULL)
    {
      LM_E(("Failed to create context from Tree : %s", details));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from tree", details, OrionldDetailsString);
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
        if (orionldContextTreat(ciP, contextArrayItemP, entityId, caPP) == false)
        {
          LM_E(("Error treating context object inside array"));
          orionldState.contextP = NULL;  // Leak?
          orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error treating context object inside array", NULL, OrionldDetailsString);
          return false;
        }
      }
      else
      {
        LM_E(("Context Array Item is not a String nor an Object, but of type '%s'", kjValueType(contextArrayItemP->type)));
        orionldState.contextP = NULL;  // Leak?
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Context Array Item of unsupported type", NULL, OrionldDetailsString);
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
      orionldState.contextP = NULL;  // Leak?
      return false;
    }

    orionldState.inlineContext.url       = (char*) "inline context";
    orionldState.inlineContext.tree      = contextNodeP;
    orionldState.inlineContext.type      = OrionldUserContext;
    orionldState.inlineContext.ignore    = false;
    orionldState.inlineContext.temporary = true;
    orionldState.inlineContext.next      = NULL;
    orionldState.contextP                = &orionldState.inlineContext;
    LM_TMP(("KZ: Set orionldState.contextP to &orionldState.inlineContext"));

    return true;
  }
  else
  {
    LM_E(("invalid JSON type of @context member"));
    orionldState.contextP = NULL;  // Leak?
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", "invalid JSON type of @context member", OrionldDetailsString);
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
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", details, OrionldDetailsString);
    return false;
  }
#endif

  if (caPP == NULL)
  {
    return true;
  }

  // Create a context attribute of the context
  ContextAttribute* caP = NULL;
  LM_T(LmtContextTreat, ("The @context is treated as an attribute"));

  // The attribute's value is either a string or a vector (compound)
  if (contextNodeP->type == KjString)
  {
    caP = new ContextAttribute("@context", "ContextString", contextNodeP->value.s);
  }
  else if (contextNodeP->type == KjArray)
  {
    // Create the Compound, just a vector of strings
    orion::CompoundValueNode* compoundP = new orion::CompoundValueNode(orion::ValueTypeVector);
    int                       siblingNo = 0;

    // Loop over the kNode vector and create the strings
    for (KjNode* contextItemNodeP = contextNodeP->value.firstChildP; contextItemNodeP != NULL; contextItemNodeP = contextItemNodeP->next)
    {
      LM_T(LmtContextTreat, ("string: %s", contextItemNodeP->value.s));
      orion::CompoundValueNode* stringNode = new orion::CompoundValueNode(compoundP, "", "", contextItemNodeP->value.s, siblingNo++, orion::ValueTypeString);

      compoundP->add(stringNode);
    }

    // Now set 'compoundP' as value of the attribute
    caP = new ContextAttribute();

    caP->type           = "ContextVector";
    caP->name           = "@context";
    caP->valueType      = orion::ValueTypeObject;  // All compounds have Object as value type (I think)
    caP->compoundValueP = compoundP;
  }
  else if (contextNodeP->type == KjObject)
  {
    orion::CompoundValueNode* compoundP = new orion::CompoundValueNode(orion::ValueTypeObject);
    int                       siblingNo = 0;

    // Loop over the kNode tree and create the strings
    for (KjNode* contextItemNodeP = contextNodeP->value.firstChildP; contextItemNodeP != NULL; contextItemNodeP = contextItemNodeP->next)
    {
      LM_T(LmtContextTreat, ("string: %s", contextItemNodeP->value.s));
      orion::CompoundValueNode* stringNode = new orion::CompoundValueNode(compoundP, "", "", contextItemNodeP->value.s, siblingNo++, orion::ValueTypeString);

      compoundP->add(stringNode);
    }

    // Now set 'compoundP' as value of the attribute
    caP = new ContextAttribute();

    caP->type           = "ContextObject";
    caP->name           = "@context";
    caP->valueType      = orion::ValueTypeObject;
    caP->compoundValueP = compoundP;
  }

  *caPP = caP;

  return caP;
}
