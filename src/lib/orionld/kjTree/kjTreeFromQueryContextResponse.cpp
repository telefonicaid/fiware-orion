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
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
}

#include "parseArgs/baStd.h"                                   // BA_FT - for debugging only
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderAdd
#include "ngsi10/QueryContextResponse.h"                       // QueryContextResponse

#include "orionld/common/orionldErrorResponse.h"               // OrionldResponseErrorType, orionldErrorResponse
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextValueLookup.h"         // orionldContextValueLookup
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/context/orionldContextListPresent.h"         // orionldContextListPresent
#include "orionld/common/httpStatusCodeToOrionldErrorType.h"   // httpStatusCodeToOrionldErrorType
#include "orionld/kjTree/kjTreeFromContextAttribute.h"         // kjTreeFromContextAttribute
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"     // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextTreeFromAttribute -
//
// What we have in ContextAttribute::value is what we need as value of "@context" in the Context Tree.
// We must create the toplevel object and the "@context" member, and give it the value of "ContextAttribute::value".
// If ContextAttribute::value is a string, then the Context Tree will be simply a string called "@context" with the value of ContextAttribute::value.
// If ContextAttribute::value is a vector ...
// If ContextAttribute::value is an object ...
//
static KjNode* orionldContextTreeFromAttribute(ConnectionInfo* ciP, ContextAttribute* caP, char** detailsP)
{
  KjNode* topNodeP = NULL;

  if (caP->valueType == orion::ValueTypeString)
  {
    LM_TMP(("It's a String!"));
    topNodeP = kjString(ciP->kjsonP, "@context", caP->stringValue.c_str());
  }
  else if (caP->valueType == orion::ValueTypeVector)
  {
    LM_TMP(("It's an Array!"));
    topNodeP = kjArray(ciP->kjsonP, "@context");

    // the vector must be of strings
    for (unsigned int ix = 0; ix < caP->compoundValueP->childV.size(); ix++)
    {
      orion::CompoundValueNode* compoundP = caP->compoundValueP->childV[ix];
      
      if (compoundP->valueType != orion::ValueTypeString)
      {
        kjFree(topNodeP);
        *detailsP = (char*) "Array member not a string";
        return NULL;
      }

      KjNode* itemNodeP = kjString(ciP->kjsonP, NULL, compoundP->stringValue.c_str());
      kjChildAdd(topNodeP, itemNodeP);
      LM_TMP(("Added array item '%s' to context", itemNodeP->value.s));
    }
  }
  else if (caP->valueType == orion::ValueTypeObject)
  {
    LM_TMP(("It's an Object!"));
    topNodeP = kjObject(ciP->kjsonP, "@context");

    // All members must be strings
    for (unsigned int ix = 0; ix < caP->compoundValueP->childV.size(); ix++)
    {
      orion::CompoundValueNode* compoundP = caP->compoundValueP->childV[ix];

      if (compoundP->valueType != orion::ValueTypeString)
      {
        kjFree(topNodeP);
        *detailsP = (char*) "Array member not a string";
        return NULL;
      }

      KjNode* itemNodeP = kjString(ciP->kjsonP, compoundP->name.c_str(), compoundP->stringValue.c_str());
      kjChildAdd(topNodeP, itemNodeP);

      LM_TMP(("Added object member '%s' == '%s' to context", itemNodeP->name, itemNodeP->value.s));
    }
  }
  else
  {
    LM_E(("Error - @context attribute value must be either String, Array, or Object"));
    return NULL;
  }

  return topNodeP;
}



// -----------------------------------------------------------------------------
//
// kjTreeFromQueryContextResponse -
//
// PARAMETERS
//   ciP        - ConnectionInfo, where all info about each request is stored
//   responseP  - The binary struct that is being converted to a KjNode tree
//
// The @context of an entity of "responseP", is the attribute named "@context" inside
// responseP->contextElementResponseVector[ix]->contextElement.ContextAttributeVector.
// The value of this context must be used to replace the long names of the items eligible for alias replacement, into their aliases.
// The context for the entity is found in the context-cache.
// If not present, it is retreived from the "@context" attribute of the entity and put in the cache
//
// Items eligible for alias replacement:
//  - Entity-Type
//  - Attr-Name
//
KjNode* kjTreeFromQueryContextResponse(ConnectionInfo* ciP, QueryContextResponse* responseP)
{
  LM_TMP(("In kjTreeFromQueryContextResponse - later will be calling orionldContextValueLookup"));
  //
  // Error?
  //
  if (responseP->errorCode.code == SccNone)
    responseP->errorCode.code = SccOk;

  if (responseP->errorCode.code != SccOk)
  {
    LM_E(("Error %d from mongoBackend", responseP->errorCode.code));
    OrionldResponseErrorType errorType = httpStatusCodeToOrionldErrorType(responseP->errorCode.code);

    orionldErrorResponseCreate(ciP, errorType, responseP->errorCode.reasonPhrase.c_str(), responseP->errorCode.details.c_str(), OrionldDetailsString);

    if (responseP->errorCode.code == SccContextElementNotFound)
      ciP->httpStatusCode = responseP->errorCode.code;

    return ciP->responseTree;    
  }

  int hits = responseP->contextElementResponseVector.size();

  if (hits == 0)  // No hit
  {
    ciP->responseTree = NULL;
    return NULL;
  }
  else if (hits > 1)  // More than one hit - not possible!
  {
    orionldErrorResponseCreate(ciP, OrionldInternalError, "More than one hit", ciP->wildcard[0], OrionldDetailsEntity);
    return NULL;
  }

  //
  // All good so far, one and only one context element in  the vector, and no errors anywhere
  // Now we need the @context of the entity, to use for alias replacements
  // The @context is found in the context-cache.
  // If this is the first time the context is used, we may need to retrieve the context from the entity and add it to the cache.
  // If the entity has no context, then all is ok as well.
  //
  ContextElement* ceP      = &responseP->contextElementResponseVector[0]->contextElement;
  char*           eId      = (char*) ceP->entityId.id.c_str();
  OrionldContext* contextP = orionldContextLookup(ceP->entityId.id.c_str());

  LM_TMP(("Getting the @context for the entity '%s'", eId));
  
  if (contextP == NULL)
  {
    LM_TMP(("The @context for '%s' was not found in the cache - adding it", eId));
    ContextAttribute* contextAttributeP = ceP->contextAttributeVector.lookup("@context");

    if (contextAttributeP != NULL)
    {
      //
      // Now we need to convert the ContextAttribute "@context" into a OrionldContext
      //
      // Let's compare a Context Tree with the ContextAttribute "@context"
      //
      // Context Tree:
      //   ----------------------------------------------
      //   {
      //     "@context": {} | [] | ""
      //   }
      //
      // ContextAttribute:
      //   ----------------------------------------------
      //   "@context": {
      //     "type": "xxx",
      //     "value": {} | [] | ""
      //
      // What we have in ContextAttribute::value is what we need as value of "@context" in the Context Tree.
      // We must create the toplevel object and the "@context" member, and give it the value of "ContextAttribute::value".
      // If ContextAttribute::value is a string, then the Context Tree will be simply a string called "@context" with the value of ContextAttribute::value.
      // If ContextAttribute::value is a vector ...
      // If ContextAttribute::value is an object ...
      // The function orionldContextTreeFromAttribute does just this
      //

      LM_TMP(("Found an attribute called context @context for entity '%s'", eId));
      char*    details;
      LM_TMP(("Creating a KjNode tree for the @context attribute"));
      KjNode*  contextTree = orionldContextTreeFromAttribute(ciP, contextAttributeP, &details);

      if (contextTree == NULL)
      {
        LM_E(("Unable to create context tree for @context attribute of entity '%s': %s", eId, details));
        orionldErrorResponseCreate(ciP, OrionldInternalError, "Unable to create context tree for @context attribute", details, OrionldDetailsEntity);
        return NULL;
      }
      LM_TMP(("Created a KjNode tree for the @context attribute. Now creating a orionldContext for the tree"));

      contextP = orionldContextCreateFromTree(contextTree, eId, OrionldUserContext, &details);
      if (contextP == NULL)
      {
        LM_E(("Unable to create context from tree: %s", details));
        orionldErrorResponseCreate(ciP, OrionldInternalError, "Unable to create context from tree", details, OrionldDetailsEntity);
        return NULL;
      }

      LM_TMP(("Inserting the new context in the context-cache"));
      orionldContextListInsert(contextP);
      orionldContextListPresent();
    }
  }


  //
  // Time to create the KjNode tree
  //
  KjNode*  top = kjObject(NULL, NULL);
  KjNode*  nodeP;
  
  // id
  nodeP = kjString(ciP->kjsonP, "id", ceP->entityId.id.c_str());
  // FIXME: uridecode nodeP->value.s
  kjChildAdd(top, nodeP);


  // type
  if (ceP->entityId.type != "")
  {
    nodeP = NULL;

    LM_TMP(("=================== Reverse alias-search for Entity-Type '%s'", ceP->entityId.type.c_str()));

    // Is it the default URL ?
    if (orionldDefaultUrlLen != -1)
    {
      if (strncmp(ceP->entityId.type.c_str(), orionldDefaultUrl, orionldDefaultUrlLen) == 0)
      {
        nodeP = kjString(ciP->kjsonP, "type", &ceP->entityId.type.c_str()[orionldDefaultUrlLen]);
        if (nodeP == NULL)
        {
          LM_E(("out of memory"));
          orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node", "out of memory", OrionldDetailsEntity);
          return NULL;
        }
      }
    }

    if (nodeP == NULL)
    {
      LM_TMP(("Calling orionldContextValueLookup for %s", ceP->entityId.type.c_str()));
      KjNode* aliasNodeP = orionldContextValueLookup(contextP, ceP->entityId.type.c_str());

      if (aliasNodeP != NULL)
      {
        LM_TMP(("Found the alias: '%s' => '%s'", ceP->entityId.type.c_str(), aliasNodeP->name));
        nodeP = kjString(ciP->kjsonP, "type", aliasNodeP->name);
      }
      else
      {
        LM_TMP(("No alias found, keeping long name '%s'", ceP->entityId.type.c_str()));
        nodeP = kjString(ciP->kjsonP, "type", ceP->entityId.type.c_str());
      }

      if (nodeP == NULL)
      {
        LM_E(("out of memory"));
        orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node", "out of memory", OrionldDetailsEntity);
        return NULL;
      }      
    }
    
    kjChildAdd(top, nodeP);
  }

  else
    LM_TMP(("NOT Calling orionldContextValueLookup for entity Type as it is EMPTY!!!"));

  //
  // Attributes, including @context
  //
  ContextAttribute* contextAttrP = NULL;

  for (unsigned int aIx = 0; aIx < ceP->contextAttributeVector.size(); aIx++)
  {
    ContextAttribute* aP       = ceP->contextAttributeVector[aIx];
    char*             attrName = (char*) aP->name.c_str();
    KjNode*           aTop;

    if (strcmp(attrName, "@context") == 0)
    {
      contextAttrP = aP;
      continue;
    }

    // Is it the default URL ?
    if ((orionldDefaultUrlLen != -1) && (strncmp(attrName, orionldDefaultUrl, orionldDefaultUrlLen) == 0))
    {
      attrName = &attrName[orionldDefaultUrlLen];
    }
    else
    {
      //
      // Lookup alias for the Attribute Name
      //
      KjNode* aliasNodeP = orionldContextValueLookup(contextP, aP->name.c_str());
    
      if (aliasNodeP != NULL)
        attrName = aliasNodeP->name;
    }

    aTop = kjObject(ciP->kjsonP, attrName);
    if (aTop == NULL)
    {
      LM_E(("Error creating a KjNode Object"));
      orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node", "out of memory", OrionldDetailsEntity);
      return NULL;
    }

    // type
    if (aP->type != "")
    {
      nodeP = kjString(ciP->kjsonP, "type", aP->type.c_str());
      if (nodeP == NULL)
      {
        LM_E(("Error creating a KjNode String"));
        orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node", "out of memory", OrionldDetailsEntity);
        return NULL;
      }

      kjChildAdd(aTop, nodeP);
    }

    // value
    switch (aP->valueType)
    {
    case orion::ValueTypeString:
      nodeP = kjString(ciP->kjsonP, "value", aP->stringValue.c_str());
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeNumber:
      nodeP = kjFloat(ciP->kjsonP, "value", aP->numberValue);  // FIXME: kjInteger or kjFloat ...
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeBoolean:
      nodeP = kjBoolean(ciP->kjsonP, "value", (KBool) aP->boolValue);
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeNull:
      nodeP = kjNull(ciP->kjsonP, "value");
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeNotGiven:
      nodeP = kjString(ciP->kjsonP, "value", "UNKNOWN TYPE");
      kjChildAdd(aTop, nodeP);
      break;

    case orion::ValueTypeVector:
    case orion::ValueTypeObject:
      nodeP = kjString(ciP->kjsonP, "value", "compounds for later ...");
      kjChildAdd(aTop, nodeP);
      break;
    }

    kjChildAdd(top, aTop);  // Adding the attribute to the tree
  }


  //
  // Set MIME Type to JSONLD if JSONLD is in the Accepot header of the incoming request
  //
  if (ciP->httpHeaders.acceptJsonld == true)
  {
    ciP->outMimeType = JSONLD;
    LM_TMP(("KZ: ciP->outMimeType = JSONLD"));
  }


  //
  // If no context inside attribute list, then the default context has been used
  //
  if (contextAttrP == NULL)
  {
    nodeP = kjString(ciP->kjsonP, "@context", orionldCoreContext.url);

    LM_TMP(("KZ: ciP->httpHeaders.acceptJsonld == %s", BA_FT(ciP->httpHeaders.acceptJsonld)));
    LM_TMP(("KZ: ciP->httpHeaders.acceptJson   == %s", BA_FT(ciP->httpHeaders.acceptJson)));
    if (ciP->httpHeaders.acceptJsonld == true)
    {
      kjChildAdd(top, nodeP);
    }
    else
      httpHeaderAdd(ciP, "Link", orionldCoreContext.url);  // Should we send back the Link if Core Context?
  }
  else
  {
    if (ciP->httpHeaders.acceptJsonld == false)
    {
      if (contextAttrP->valueType == orion::ValueTypeString)
      {
        httpHeaderAdd(ciP, "Link", contextAttrP->stringValue.c_str());
      }
      else
      {
        httpHeaderAdd(ciP, "Link", "Implement Context-Servicing for orionld");
      }
    }
    else
    {
      switch (contextAttrP->valueType)
      {
      case orion::ValueTypeString:
        nodeP = kjString(ciP->kjsonP, "@context", contextAttrP->stringValue.c_str());
        kjChildAdd(top, nodeP);
        break;

      case orion::ValueTypeVector:
        nodeP = kjArray(ciP->kjsonP, "@context");
        kjChildAdd(top, nodeP);

        for (unsigned int ix = 0; ix < contextAttrP->compoundValueP->childV.size(); ix++)
        {
          orion::CompoundValueNode*  compoundP     = contextAttrP->compoundValueP->childV[ix];
          KjNode*                    contextItemP  = kjString(ciP->kjsonP, NULL, compoundP->stringValue.c_str());
          kjChildAdd(nodeP, contextItemP);
        }
        break;

      default:
        orionldErrorResponseCreate(ciP, OrionldInternalError, "invalid context", "not a string nor an array", OrionldDetailsString);
        // FIXME: leaks!!! (Call kjFree(top))?
        return ciP->responseTree;
      }
    }
  }
  return top;
}
