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

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/rest.h"                                         // restPortGet
#include "ngsi/ContextAttribute.h"                             // ContextAttribute
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/rest/orionldServiceInit.h"                   // orionldHostNameLen
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextCreateFromUrl.h"       // orionldContextCreateFromUrl
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextAdd.h"                 // orionldContextAdd
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
#if 1  // TMP - perhaps it fixes the multiple copies of contexts ... ?
  if ((ciP->contextP = orionldContextLookup(contextNodeP->name)) != NULL)
  {
    LM_TMP(("Context '%s' already exists", contextNodeP->name));
    return true;
  }
#endif

  if (contextNodeP->type == KjString)
  {
    char* details;

    if ((ciP->contextP = orionldContextCreateFromUrl(ciP, contextNodeP->value.s, OrionldUserContext, &details)) == NULL)
    {
      LM_E(("Failed to create context from URL: %s", details));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from URL", details, OrionldDetailsString);
      return false;
    }

    ciP->contextToBeFreed = false;  // context has been added to public list - must not be freed
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

    sprintf(linkPath, "http://%s:%d/ngsi-ld/ex/v1/contexts/%s", orionldHostName, restPortGet(), entityId);

    ciP->contextP = orionldContextCreateFromTree(contextNodeP, linkPath, OrionldUserContext, &details);

    if (linkPath != linkPathV)
      free(linkPath);  // orionldContextCreateFromTree strdups the URL

    if (ciP->contextP == NULL)
    {
      LM_E(("Failed to create context from Tree : %s", details));
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "failure to create context from tree", details, OrionldDetailsString);
      return false;
    }

    //
    // The context containing a vector of X context strings (URLs) must be freed
    // The downloaded contexts though are added to the public list and will not be freed)
    //
    ciP->contextToBeFreed = true;

    for (KjNode* contextStringNodeP = contextNodeP->children; contextStringNodeP != NULL; contextStringNodeP = contextStringNodeP->next)
    {
      if (contextStringNodeP->type != KjString)
      {
        LM_E(("Context Array Item is not a JSON String, but of type '%s'", kjValueType(contextStringNodeP->type)));
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Context Array Item is not a JSON String", NULL, OrionldDetailsString);
        
        return false;
      }

      if (contextItemNodeTreat(ciP, contextStringNodeP->value.s) == NULL)
      {
        LM_E(("contextItemNodeTreat failed"));
        // Error payload set by contextItemNodeTreat
        return false;
      }
    }
  }
  else if (contextNodeP->type == KjObject)
  {
    // FIXME: seems like an inline context - not supported for now
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", "inline contexts not supported in current version of orionld", OrionldDetailsString);
    LM_E(("inline contexts not supported in current version of orionld"));
    return false;
  }
  else
  {
    LM_E(("invalid JSON type of @context member"));
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

  if (orionldUserContextKeyValuesCheck(ciP->contextP->tree, ciP->contextP->url, &details) == false)
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid context", details, OrionldDetailsString);
    return false;
  }
#endif

  if (caPP == NULL)
    return true;

  // Create a context attribute of the context
  ContextAttribute* caP;
  LM_T(LmtContextTreat, ("The @context is treated as an attribute"));

  // The attribute's value is either a string or a vector (compound)
  if (contextNodeP->type == KjString)
  {
    caP = new ContextAttribute("@context", "ContextString", contextNodeP->value.s);
  }
  else
  {
    // Create the Compound, just a vector of strings
    orion::CompoundValueNode* compoundP = new orion::CompoundValueNode(orion::ValueTypeVector);
    int                       siblingNo = 0;

    // Loop over the kNode vector and create the strings
    for (KjNode* contextItemNodeP = contextNodeP->children; contextItemNodeP != NULL; contextItemNodeP = contextItemNodeP->next)
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

  *caPP = caP;

  return caP;
}
