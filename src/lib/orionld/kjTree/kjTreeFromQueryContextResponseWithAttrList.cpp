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

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderAdd, httpHeaderLinkAdd
#include "ngsi10/QueryContextResponse.h"                       // QueryContextResponse

#include "orionld/common/orionldErrorResponse.h"               // OrionldResponseErrorType, orionldErrorResponse
#include "orionld/common/httpStatusCodeToOrionldErrorType.h"   // httpStatusCodeToOrionldErrorType
#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "common/string.h"                                     // FT
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldContextValueLookup.h"         // orionldContextValueLookup
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/context/orionldContextListPresent.h"         // orionldContextListPresent
#include "orionld/kjTree/kjTreeFromContextAttribute.h"         // kjTreeFromContextAttribute
#include "orionld/kjTree/kjTreeFromContextContextAttribute.h"  // kjTreeFromContextContextAttribute
#include "orionld/kjTree/kjTreeFromCompoundValue.h"            // kjTreeFromCompoundValue
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"     // Own interface



// FIXME: Move function to separate file
extern bool orionldSysAttrs(ConnectionInfo* ciP, double creDate, double modDate, KjNode* containerP);



// -----------------------------------------------------------------------------
//
// kjTreeFromQueryContextResponseWithAttrList -
//
// PARAMETERS
//   ciP        - ConnectionInfo, where all info about each request is stored
//   oneHit     - if TRUE, create a JSON object, else a JSON Array
//   keyValues  - if TRUE, omit details of the attributes
//   responseP  - The binary struct that is being converted to a KjNode tree
//
// The @context of an entity of "responseP", is the attribute named "@context" inside
// responseP->contextElementResponseVector[ix]->contextElement.ContextAttributeVector.
// The value of this context must be used to replace the long names of the items eligible for alias replacement, into their aliases.
// The context for the entity is found in the context-cache.
// If not present, it is retreived from the "@context" attribute of the entity and put in the cache
//
KjNode* kjTreeFromQueryContextResponseWithAttrList(ConnectionInfo* ciP, bool oneHit, const char* attrList, bool keyValues, QueryContextResponse* responseP)
{
  char* details  = NULL;
  bool  sysAttrs = ciP->uriParamOptions["sysAttrs"];

  //
  // No hits when "oneHit == false" is not an error.
  // We just return an empty array
  //
  if ((oneHit == false) && (responseP->contextElementResponseVector.size() == 0))
  {
    ciP->responseTree = kjArray(orionldState.kjsonP, NULL);
    ciP->httpStatusCode = SccOk;
    return ciP->responseTree;
  }

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
    if (oneHit == false)
      ciP->responseTree = kjArray(orionldState.kjsonP, NULL);
    else
      ciP->responseTree = NULL;

    return ciP->responseTree;
  }
  else if ((hits > 1) && (oneHit == true))  // More than one hit - not possible!
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
  KjNode*  root = (oneHit == true)? kjObject(NULL, NULL) : kjArray(NULL, NULL);
  KjNode*  top  = NULL;

  if (oneHit == true)
    top = root;

  LM_TMP(("oneHit: %s", FT(oneHit)));
  
  for (int ix = 0; ix < hits; ix++)
  {
    ContextElement* ceP      = &responseP->contextElementResponseVector[ix]->contextElement;
    char*           eId      = (char*) ceP->entityId.id.c_str();
    OrionldContext* contextP = orionldContextLookup(ceP->entityId.id.c_str());

    if (oneHit == false)
    {
      top = kjObject(NULL, NULL);
      kjChildAdd(root, top);
    }

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
        // The function kjTreeFromContextContextAttribute does just this
        //

        LM_TMP(("Found an attribute called context @context for entity '%s'", eId));
        char*    details;
        KjNode*  contextTree = kjTreeFromContextContextAttribute(ciP, contextAttributeP, &details);

        if (contextTree == NULL)
        {
          LM_E(("Unable to create context tree for @context attribute of entity '%s': %s", eId, details));
          orionldErrorResponseCreate(ciP, OrionldInternalError, "Unable to create context tree for @context attribute", details, OrionldDetailsEntity);
          return NULL;
        }

        contextP = orionldContextCreateFromTree(contextTree, eId, OrionldUserContext, &details);
        if (contextP == NULL)
        {
          LM_E(("Unable to create context from tree: %s", details));
          orionldErrorResponseCreate(ciP, OrionldInternalError, "Unable to create context from tree", details, OrionldDetailsEntity);
          return NULL;
        }

        orionldContextListInsert(contextP);  // Inserting the context of an Entity, after not finding it in the context list
        orionldContextListPresent(ciP);
      }
    }


    //
    // Time to create the KjNode tree
    //
    KjNode*  nodeP;

    // id
    nodeP = kjString(orionldState.kjsonP, "id", ceP->entityId.id.c_str());
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
          nodeP = kjString(orionldState.kjsonP, "type", &ceP->entityId.type.c_str()[orionldDefaultUrlLen]);
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
        bool    useStringValue = false;
        KjNode* aliasNodeP     = orionldContextValueLookup(contextP, ceP->entityId.type.c_str(), &useStringValue);

        if (aliasNodeP != NULL)
        {
          char* alias = (useStringValue == false)? aliasNodeP->name: aliasNodeP->value.s;

          LM_TMP(("Found the alias: '%s' => '%s'", ceP->entityId.type.c_str(), alias));
          nodeP = kjString(orionldState.kjsonP, "type", alias);
        }
        else
        {
          LM_TMP(("No alias found, keeping long name '%s'", ceP->entityId.type.c_str()));
          nodeP = kjString(orionldState.kjsonP, "type", ceP->entityId.type.c_str());
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

    // System Attributes?
    if (sysAttrs == true)
    {
      if (orionldSysAttrs(ciP, ceP->entityId.creDate, ceP->entityId.modDate, top) == false)
      {
        LM_E(("sysAttrs error"));
        return NULL;
      }
    }


    //
    // Attributes, including @context
    //
    // FIXME: Use kjTreeFromContextAttribute() !!!
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
        bool    useStringValue = false;
        KjNode* aliasNodeP     = orionldContextValueLookup(contextP, aP->name.c_str(), &useStringValue);

        if (aliasNodeP != NULL)
          attrName = (useStringValue == false)? aliasNodeP->name : aliasNodeP->value.s;
      }

      char* match;
      if ((match = (char*) strstr(attrList, aP->name.c_str())) == NULL)
      {
        continue;
      }

      // Need to check ",{attr name}," also
      if ((match[-1] != ',') || (match[strlen(aP->name.c_str())] != ','))
      {
        continue;
      }

      if (keyValues)
      {
        // If keyValues, then just the value of the attribute is to be rendered (built)
        switch (aP->valueType)
        {
        case orion::ValueTypeNumber:    aTop = kjFloat(orionldState.kjsonP, attrName,   aP->numberValue);          break;
        case orion::ValueTypeBoolean:   aTop = kjBoolean(orionldState.kjsonP, attrName, aP->boolValue);            break;
        case orion::ValueTypeString:    aTop = kjString(orionldState.kjsonP, attrName,  aP->stringValue.c_str());  break;
        case orion::ValueTypeNull:      aTop = kjNull(orionldState.kjsonP, attrName);                              break;
        case orion::ValueTypeVector:
        case orion::ValueTypeObject:
          aTop = (aP->compoundValueP->valueType == orion::ValueTypeVector)? kjArray(orionldState.kjsonP, attrName) : kjObject(orionldState.kjsonP, attrName);

          if (aTop != NULL)
          {
            if (kjTreeFromCompoundValue(aP->compoundValueP, aTop, &details) == NULL)
            {
              LM_E(("kjTreeFromCompoundValue: %s", details));
              orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node from compound value", details, OrionldDetailsEntity);
              return NULL;
            }
          }
          break;

        case orion::ValueTypeNotGiven:
          orionldErrorResponseCreate(ciP, OrionldInternalError, "invalid internal JSON type for Context Atribute", NULL, OrionldDetailsEntity);
          break;
        }

        if (aTop == NULL)
        {
          LM_E(("kjTreeFromCompoundValue: %s", details));
          orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node for compound value", "out of memory", OrionldDetailsEntity);
          return NULL;
        }

        kjChildAdd(top, aTop);    // Adding the attribute to the tree
      }
      else
      {
        aTop = kjObject(orionldState.kjsonP, attrName);
        if (aTop == NULL)
        {
          LM_E(("Error creating a KjNode Object"));
          orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node", "out of memory", OrionldDetailsEntity);
          return NULL;
        }

        // type
        if (aP->type != "")
        {
          nodeP = kjString(orionldState.kjsonP, "type", aP->type.c_str());
          if (nodeP == NULL)
          {
            LM_E(("Error creating a KjNode String"));
            orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node", "out of memory", OrionldDetailsEntity);
            return NULL;
          }

          kjChildAdd(aTop, nodeP);
        }

        // value
        const char*  valueFieldName = (aP->type == "Relationship")? "object" : "value";

        LM_TMP(("KZ: valueFieldName: '%s' for attr %s", valueFieldName, aP->name.c_str()));
        switch (aP->valueType)
        {
        case orion::ValueTypeNumber:
          LM_TMP(("KZ: it's a number. "));
          if (SCOMPARE11(attrName, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
          {
            char   date[128];
            char*  details;

            if (numberToDate((time_t) aP->numberValue, date, sizeof(date), &details) == false)
            {
              LM_E(("Error creating a stringified date"));
              orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create a stringified date", details, OrionldDetailsEntity);
              return NULL;
            }

            nodeP = kjString(ciP->kjsonP, "observedAt", date);
          }
          else
            nodeP = kjFloat(ciP->kjsonP, valueFieldName, aP->numberValue);
          break;

        case orion::ValueTypeString:    nodeP = kjString(orionldState.kjsonP, valueFieldName, aP->stringValue.c_str());      break;
        case orion::ValueTypeBoolean:   nodeP = kjBoolean(orionldState.kjsonP, valueFieldName, (KBool) aP->boolValue);       break;
        case orion::ValueTypeNull:      nodeP = kjNull(orionldState.kjsonP, valueFieldName);                                 break;
        case orion::ValueTypeNotGiven:  nodeP = kjString(orionldState.kjsonP, valueFieldName, "UNKNOWN TYPE");               break;

        case orion::ValueTypeVector:
        case orion::ValueTypeObject:
          nodeP = (aP->valueType == orion::ValueTypeVector)? kjArray(orionldState.kjsonP, valueFieldName) : kjObject(orionldState.kjsonP, valueFieldName);
          if (nodeP == NULL)
          {
            LM_E(("kjTreeFromCompoundValue: %s", details));
            orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node for compound value", "out of memory", OrionldDetailsEntity);
            return NULL;
          }

          if (kjTreeFromCompoundValue(aP->compoundValueP, nodeP, &details) == NULL)
          {
            LM_E(("kjTreeFromCompoundValue: %s", details));
            orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create tree node from compound value", details, OrionldDetailsEntity);
            return NULL;
          }
          break;
        }

        kjChildAdd(aTop, nodeP);  // Add the value to the attribute
        kjChildAdd(top, aTop);    // Adding the attribute to the tree

        // System Attributes?
        if (sysAttrs == true)
        {
          if (orionldSysAttrs(ciP, aP->creDate, aP->modDate, aTop) == false)
          {
            LM_E(("sysAttrs error"));
            return NULL;
          }
        }

        // Metadata
        for (unsigned int ix = 0; ix < aP->metadataVector.size(); ix++)
        {
          //
          // Metadata with "type" != "" are built as Objects with type+value/object
          //
          Metadata* mdP     = aP->metadataVector[ix];
          char*     mdName  = (char*) mdP->name.c_str();

          if (mdP->type != "")
          {
            const char*  valueFieldName = (mdP->type == "Relationship")? "object" : "value";
            KjNode*      typeP;
            KjNode*      valueP = NULL;

            nodeP = kjObject(orionldState.kjsonP, mdName);

            typeP = kjString(orionldState.kjsonP, "type", mdP->type.c_str());
            kjChildAdd(nodeP, typeP);

            details = NULL;
            switch (mdP->valueType)
            {
            case orion::ValueTypeNumber:
              if (SCOMPARE11(mdName, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
              {
                char   date[128];
                char*  details;

                if (numberToDate((time_t) mdP->numberValue, date, sizeof(date), &details) == false)
                {
                  LM_E(("Error creating a stringified date"));
                  orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create a stringified date", details, OrionldDetailsEntity);
                  return NULL;
                }

                valueP = kjString(ciP->kjsonP, "observedAt", date);
              }
              else
                valueP = kjFloat(ciP->kjsonP, valueFieldName, mdP->numberValue);
              break;

            case orion::ValueTypeString:   valueP = kjString(orionldState.kjsonP, valueFieldName, mdP->stringValue.c_str());   break;
            case orion::ValueTypeBoolean:  valueP = kjBoolean(orionldState.kjsonP, valueFieldName, mdP->boolValue);            break;
            case orion::ValueTypeNull:     valueP = kjNull(orionldState.kjsonP, valueFieldName);                               break;
            case orion::ValueTypeNotGiven: valueP = kjString(orionldState.kjsonP, valueFieldName, "UNKNOWN TYPE IN MONGODB");  break;

            case orion::ValueTypeObject:   valueP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, &details); valueP->name = (char*) "value"; break;
            case orion::ValueTypeVector:   valueP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, &details); valueP->name = (char*) "value"; break;
            }

            kjChildAdd(nodeP, valueP);
          }
          else
          {
            details = NULL;
            switch (mdP->valueType)
            {
            case orion::ValueTypeNumber:
              if (SCOMPARE11(mdName, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
              {
                char   date[128];
                char*  details;

                if (numberToDate((time_t) mdP->numberValue, date, sizeof(date), &details) == false)
                {
                  LM_E(("Error creating a stringified date"));
                  orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create a stringified date", details, OrionldDetailsEntity);
                  return NULL;
                }

                nodeP = kjString(ciP->kjsonP, "observedAt", date);
              }
              else
                nodeP = kjFloat(ciP->kjsonP, valueFieldName, mdP->numberValue);
              break;

            case orion::ValueTypeString:   nodeP = kjString(orionldState.kjsonP, mdName, mdP->stringValue.c_str());            break;
            case orion::ValueTypeBoolean:  nodeP = kjBoolean(orionldState.kjsonP, mdName, mdP->boolValue);                     break;
            case orion::ValueTypeNull:     nodeP = kjNull(orionldState.kjsonP, mdName);                                        break;
            case orion::ValueTypeNotGiven: nodeP = kjString(orionldState.kjsonP, mdName, "UNKNOWN TYPE IN MONGODB");           break;

            case orion::ValueTypeObject:   nodeP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, &details);  nodeP->name = (char*) "value"; break;
            case orion::ValueTypeVector:   nodeP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, &details);  nodeP->name = (char*) "value"; break;
            }
          }

          if (nodeP == NULL)
            LM_E(("Error in creation of KjNode for metadata (%s)", (details != NULL)? details : "no details"));
          else
            kjChildAdd(aTop, nodeP);
        }
      }
    }

    //
    // Set MIME Type to JSONLD if JSONLD is in the Accept header of the incoming request
    // FIXME: probably no longer necessary - done in orionldMhdConnectionTreat.cpp::acceptHeaderCheck()
    //
    if (ciP->httpHeaders.acceptJsonld == true)
    {
      ciP->outMimeType = JSONLD;
    }


    //
    // If no context inside attribute list, then the default context has been used
    //
    if (contextAttrP == NULL)
    {
      if (ciP->httpHeaders.acceptJsonld == true)
      {
        nodeP = kjString(orionldState.kjsonP, "@context", orionldDefaultContext.url);
        kjChildAdd(top, nodeP);
      }
      else
        httpHeaderLinkAdd(ciP, &orionldDefaultContext, NULL);
    }
    else
    {
      if (ciP->httpHeaders.acceptJsonld == false)
      {
        if (contextAttrP->valueType == orion::ValueTypeString)
        {
          OrionldContext context = { (char*) contextAttrP->stringValue.c_str(), NULL, OrionldUserContext, false, NULL };
          httpHeaderLinkAdd(ciP, &context, NULL);
        }
        else
        {
          // FIXME: Implement Context-Servicing for orionld
          //
          // If we get here, the context is compound, and can't be returned in the Link HTTP Header.
          // We now need to create a context in the broker, able to serve it, and in the Link HTTP Header
          // return this Link
          //
          httpHeaderLinkAdd(ciP, NULL, NULL);
        }
      }
      else
      {
        if (contextAttrP->valueType == orion::ValueTypeString)
        {
          nodeP = kjString(orionldState.kjsonP, "@context", contextAttrP->stringValue.c_str());
          kjChildAdd(top, nodeP);
        }
        else if (contextAttrP->compoundValueP != NULL)
        {
          if (contextAttrP->compoundValueP->valueType == orion::ValueTypeVector)
          {
            nodeP = kjArray(orionldState.kjsonP, "@context");
            kjChildAdd(top, nodeP);

            for (unsigned int ix = 0; ix < contextAttrP->compoundValueP->childV.size(); ix++)
            {
              orion::CompoundValueNode*  compoundP     = contextAttrP->compoundValueP->childV[ix];
              KjNode*                    contextItemP  = kjString(orionldState.kjsonP, NULL, compoundP->stringValue.c_str());
              kjChildAdd(nodeP, contextItemP);
            }
          }
          else
          {
            orionldErrorResponseCreate(ciP, OrionldInternalError, "invalid context", "inline contexts not supported - wait it's coming ...", OrionldDetailsString);
            return ciP->responseTree;
          }
        }
        else
        {
          orionldErrorResponseCreate(ciP, OrionldInternalError, "invalid context", "not a string nor an array", OrionldDetailsString);
          return ciP->responseTree;
        }
      }
    }
  }

  return root;
}
