/*
*
* Copyright 2018 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "common/string.h"                                     // FT
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "ngsi10/QueryContextResponse.h"                       // QueryContextResponse

#include "orionld/common/orionldErrorResponse.h"               // OrionldResponseErrorType, orionldErrorResponse
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/common/httpStatusCodeToOrionldErrorType.h"   // httpStatusCodeToOrionldErrorType
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/kjTree/kjTreeFromContextAttribute.h"         // kjTreeFromContextAttribute
#include "orionld/kjTree/kjTreeFromCompoundValue.h"            // kjTreeFromCompoundValue
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"     // Own interface



// -----------------------------------------------------------------------------
//
// inAttrList -
//
static bool inAttrList(const char* attrName, char** attrListExpanded, int attrsInAttrList)
{
  for (int ix = 0; ix < attrsInAttrList; ix++)
  {
    if (strcmp(attrName, attrListExpanded[ix]) == 0)
      return true;
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// attrListParseAndExpand -
//
static void attrListParseAndExpand(int* attrsInAttrListP, char*** attrListExpandedVecP, char* attrList)
{
  int   attrs = 0;
  char* cP;

  //
  // attrList is a comma-separated list.
  // First we need to find out how many attributes are in the list (== number of commas + 1)
  //
  if (*attrList != 0)
  {
    cP = attrList;
    while (*cP != 0)
    {
      if (*cP == ',')
      {
        if (cP[1] != 0)  // If the comma is the last character - then there is no no next attribute
          ++attrs;
      }

      ++cP;
    }
  }

  ++attrs;  // Number of attributes == number of commas + 1


  //
  // Allocate room for the attribute name pointers
  //
  char** expandedV = (char**) kaAlloc(&orionldState.kalloc, attrs * sizeof(char*));

  //
  // And, parse attrList, to make the items in the vector point to each attribute name
  //

  // The first one is given:
  expandedV[0] = attrList;

  int aIx = 1;
  cP = attrList;
  while (*cP != 0)
  {
    if (*cP == ',')
    {
      *cP = 0;

      if (cP[1] != 0)  // If the comma is the last character - then there is no no next attribute
      {
        ++cP;
        expandedV[aIx++] = cP;
      }
      else
        ++cP;
    }
    else
      ++cP;
  }


  //
  // Expand attribute names, overwriting the shortnames
  //
  for (int ix = 0; ix < attrs; ix++)
  {
    expandedV[ix] = orionldContextItemExpand(orionldState.contextP, expandedV[ix], NULL, true, NULL);
  }

  *attrListExpandedVecP = expandedV;
  *attrsInAttrListP     = attrs;
}



// -----------------------------------------------------------------------------
//
// orionldSysAttrs -
//
bool orionldSysAttrs(double creDate, double modDate, KjNode* containerP)
{
  char     date[128];
  char*    details;
  KjNode*  nodeP;

  // FIXME: Always "keyValues" for 'createdAt' and 'modifiedAt' ?

  // createdAt
  if (numberToDate((time_t) creDate, date, sizeof(date), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'createdAt'"));
    orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified createdAt date", details);
    return false;
  }

  nodeP = kjString(orionldState.kjsonP, "createdAt", date);
  kjChildAdd(containerP, nodeP);

  // modifiedAt
  if (numberToDate((time_t) modDate, date, sizeof(date), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'modifiedAt'"));
    orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified modifiedAt date", details);
    return false;
  }

  nodeP = kjString(orionldState.kjsonP, "modifiedAt", date);
  kjChildAdd(containerP, nodeP);

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeFromQueryContextResponse -
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
KjNode* kjTreeFromQueryContextResponse(ConnectionInfo* ciP, bool oneHit, char* attrList, bool keyValues, QueryContextResponse* responseP)
{
  char* details  = NULL;
  bool  sysAttrs = ciP->uriParamOptions["sysAttrs"];


  //
  // No hits when "oneHit == false" is not an error.
  // We just return an empty array
  //
  if ((oneHit == false) && (responseP->contextElementResponseVector.size() == 0))
  {
    orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);
    orionldState.httpStatusCode = SccOk;

    return orionldState.responseTree;
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

    orionldErrorResponseCreate(errorType, responseP->errorCode.reasonPhrase.c_str(), responseP->errorCode.details.c_str());

    if (responseP->errorCode.code == SccContextElementNotFound)
      orionldState.httpStatusCode = responseP->errorCode.code;

    return orionldState.responseTree;
  }

  int hits = responseP->contextElementResponseVector.size();

  if (hits == 0)  // No hit
  {
    if (oneHit == false)
    {
      orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);
    }
    else
      orionldState.responseTree = NULL;

    return orionldState.responseTree;
  }
  else if ((hits > 1) && (oneHit == true))  // More than one hit - not possible!
  {
    orionldErrorResponseCreate(OrionldInternalError, "More than one hit", orionldState.wildcard[0]);
    return NULL;
  }

  //
  // All good so far, one and only one context element in  the vector, and no errors anywhere
  // Now we need the @context of the entity, to use for alias replacements
  // The @context is found in the context-cache.
  // If this is the first time the context is used, we may need to retrieve the context from the entity and add it to the cache.
  // If the entity has no context, then all is ok as well.
  //
  KjNode*  root = (oneHit == true)? kjObject(orionldState.kjsonP, NULL) : kjArray(orionldState.kjsonP, NULL);
  KjNode*  top  = NULL;

  if (oneHit == true)
    top = root;

  //
  // Expanding attrList, if present
  //
  int    attrsInAttrList  = 0;
  char** attrListExpanded = NULL;

  if (attrList)
    attrListParseAndExpand(&attrsInAttrList, &attrListExpanded, attrList);

  for (int ix = 0; ix < hits; ix++)
  {
    ContextElement* ceP = &responseP->contextElementResponseVector[ix]->contextElement;

    if (oneHit == false)
    {
      top = kjObject(orionldState.kjsonP, NULL);
      kjChildAdd(root, top);
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
      char* alias = orionldContextItemAliasLookup(orionldState.contextP, ceP->entityId.type.c_str(), NULL, NULL);

      nodeP = kjString(orionldState.kjsonP, "type", alias);
      if (nodeP == NULL)
      {
        LM_E(("out of memory"));
        orionldErrorResponseCreate(OrionldInternalError, "Unable to create tree node", "out of memory");
        return NULL;
      }

      kjChildAdd(top, nodeP);
    }


    // System Attributes?
    if (sysAttrs == true)
    {
      if (orionldSysAttrs(ceP->entityId.creDate, ceP->entityId.modDate, top) == false)
      {
        LM_E(("sysAttrs error"));
        return NULL;
      }
    }


    //
    // Attributes
    //
    // FIXME: Use kjTreeFromContextAttribute() !!!
    //
    for (unsigned int aIx = 0; aIx < ceP->contextAttributeVector.size(); aIx++)
    {
      KjNode*           aTop                 = NULL;
      ContextAttribute* aP                   = ceP->contextAttributeVector[aIx];
      const char*       attrLongName         = aP->name.c_str();
      char*             attrName;              // Attribute Short Name
      char*             valueFieldName;
      bool              valueMayBeCompacted  = false;

      attrName = orionldContextItemAliasLookup(orionldState.contextP, attrLongName, &valueMayBeCompacted, NULL);

      //
      // If URI param attrList has been used, only matching attributes should be included in the response
      //
      if ((attrListExpanded != NULL) && (inAttrList(attrLongName, attrListExpanded, attrsInAttrList) == false))
        continue;

      if (keyValues)
      {
        // If keyValues, then just the value of the attribute is to be rendered (built)
        switch (aP->valueType)
        {
        case orion::ValueTypeNull:      aTop = kjNull(orionldState.kjsonP, attrName);                              break;
        case orion::ValueTypeNumber:    aTop = kjFloat(orionldState.kjsonP, attrName,   aP->numberValue);          break;
        case orion::ValueTypeBoolean:   aTop = kjBoolean(orionldState.kjsonP, attrName, aP->boolValue);            break;
        case orion::ValueTypeString:
          if (valueMayBeCompacted == true)
          {
            char* compactedValue = orionldContextItemAliasLookup(orionldState.contextP, aP->stringValue.c_str(), NULL, NULL);
            aTop = kjString(orionldState.kjsonP, attrName, (compactedValue != NULL)? compactedValue : aP->stringValue.c_str());
          }
          else
            aTop = kjString(orionldState.kjsonP, attrName, aP->stringValue.c_str());
          break;

        case orion::ValueTypeVector:
        case orion::ValueTypeObject:
          aTop = (aP->compoundValueP->valueType == orion::ValueTypeVector)? kjArray(orionldState.kjsonP, attrName) : kjObject(orionldState.kjsonP, attrName);

          if (aTop != NULL)
          {
            if (kjTreeFromCompoundValue(aP->compoundValueP, aTop, valueMayBeCompacted, &details) == NULL)
            {
              LM_E(("kjTreeFromCompoundValue: %s", details));
              orionldErrorResponseCreate(OrionldInternalError, "Unable to create tree node from a compound value", details);
              return NULL;
            }
          }
          break;

        case orion::ValueTypeNotGiven:
          orionldErrorResponseCreate(OrionldInternalError, "Invalid internal JSON type for Context Atribute", NULL);
          break;
        }

        if (aTop == NULL)
        {
          LM_E(("kjTreeFromCompoundValue: %s", details));
          orionldErrorResponseCreate(OrionldInternalError, "Unable to create tree node for a compound value", "out of memory");
          return NULL;
        }

        kjChildAdd(top, aTop);    // Adding the attribute to the tree
      }
      else
      {
        //
        // NOT keyValues - create entire attribute tree
        //
        aTop = kjObject(orionldState.kjsonP, attrName);
        if (aTop == NULL)
        {
          LM_E(("Error creating a KjNode Object"));
          orionldErrorResponseCreate(OrionldInternalError, "Unable to create tree node", "out of memory");
          return NULL;
        }

        // type
        if (aP->type != "")
        {
          nodeP = kjString(orionldState.kjsonP, "type", aP->type.c_str());
          if (nodeP == NULL)
          {
            LM_E(("Error creating a KjNode String"));
            orionldErrorResponseCreate(OrionldInternalError, "Unable to create tree node", "out of memory");
            return NULL;
          }

          kjChildAdd(aTop, nodeP);
        }

        // value
        valueFieldName = (char*) ((aP->type == "Relationship")? "object" : "value");

        switch (aP->valueType)
        {
        case orion::ValueTypeNumber:
          if (SCOMPARE11(attrLongName, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
          {
            char   date[128];
            char*  details;

            if (numberToDate((time_t) aP->numberValue, date, sizeof(date), &details) == false)
            {
              LM_E(("Error creating a stringified date"));
              orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified observedAt date", details);
              return NULL;
            }

            nodeP = kjString(orionldState.kjsonP, "observedAt", date);
          }
          else
            nodeP = kjFloat(orionldState.kjsonP, valueFieldName, aP->numberValue);
          break;

        case orion::ValueTypeString:
          if (valueMayBeCompacted == true)
          {
            char* compactedValue = orionldContextItemAliasLookup(orionldState.contextP, aP->stringValue.c_str(), NULL, NULL);

            if (compactedValue != NULL)
              nodeP = kjString(orionldState.kjsonP, valueFieldName, compactedValue);
            else
              nodeP = kjString(orionldState.kjsonP, valueFieldName, aP->stringValue.c_str());
          }
          else
            nodeP = kjString(orionldState.kjsonP, valueFieldName, aP->stringValue.c_str());
          break;

        case orion::ValueTypeBoolean:   nodeP = kjBoolean(orionldState.kjsonP, valueFieldName, (KBool) aP->boolValue);       break;
        case orion::ValueTypeNull:      nodeP = kjNull(orionldState.kjsonP, valueFieldName);                                 break;
        case orion::ValueTypeNotGiven:  nodeP = kjString(orionldState.kjsonP, valueFieldName, "UNKNOWN TYPE");               break;

        case orion::ValueTypeVector:
        case orion::ValueTypeObject:
          nodeP = (aP->compoundValueP->valueType == orion::ValueTypeVector)? kjArray(orionldState.kjsonP, valueFieldName) : kjObject(orionldState.kjsonP, valueFieldName);
          if (nodeP == NULL)
          {
            LM_E(("kjTreeFromCompoundValue: %s", details));
            orionldErrorResponseCreate(OrionldInternalError, "Unable to create tree node for compound value", "out of memory");
            return NULL;
          }

          if (kjTreeFromCompoundValue(aP->compoundValueP, nodeP, valueMayBeCompacted, &details) == NULL)
          {
            LM_E(("kjTreeFromCompoundValue: %s", details));
            orionldErrorResponseCreate(OrionldInternalError, "Unable to create tree node from compound value", details);
            return NULL;
          }
          break;
        }

        kjChildAdd(aTop, nodeP);  // Add the value to the attribute
        kjChildAdd(top, aTop);    // Adding the attribute to the tree

        // System Attributes?
        if (sysAttrs == true)
        {
          if (orionldSysAttrs(aP->creDate, aP->modDate, aTop) == false)
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
          Metadata* mdP                  = aP->metadataVector[ix];
          char*     mdName               = (char*) mdP->name.c_str();
          bool      valueMayBeCompacted  = false;

          if ((strcmp(mdName, "observedAt") != 0) &&
              (strcmp(mdName, "createdAt")  != 0) &&
              (strcmp(mdName, "modifiedAt") != 0))
          {
            //
            // Looking up short name for the sub-attribute
            //
            mdName = orionldContextItemAliasLookup(orionldState.contextP, mdName, &valueMayBeCompacted, NULL);
          }

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
                  orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified observedAt date", details);
                  return NULL;
                }

                valueP = kjString(orionldState.kjsonP, "observedAt", date);
              }
              else
                valueP = kjFloat(orionldState.kjsonP, valueFieldName, mdP->numberValue);
              break;

            case orion::ValueTypeString:
              if (valueMayBeCompacted == true)
              {
                char* compactedValue = orionldContextItemAliasLookup(orionldState.contextP, mdP->stringValue.c_str(), NULL, NULL);

                if (compactedValue != NULL)
                  valueP = kjString(orionldState.kjsonP, valueFieldName, compactedValue);
                else
                  valueP = kjString(orionldState.kjsonP, valueFieldName, mdP->stringValue.c_str());
              }
              else
                valueP = kjString(orionldState.kjsonP, valueFieldName, mdP->stringValue.c_str());
              break;

            case orion::ValueTypeBoolean:  valueP = kjBoolean(orionldState.kjsonP, valueFieldName, mdP->boolValue);              break;
            case orion::ValueTypeNull:     valueP = kjNull(orionldState.kjsonP, valueFieldName);                                 break;
            case orion::ValueTypeNotGiven: valueP = kjString(orionldState.kjsonP, valueFieldName, "UNKNOWN TYPE IN MONGODB 1");  break;

            case orion::ValueTypeObject:   valueP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, valueMayBeCompacted, &details); valueP->name = (char*) "value"; break;
            case orion::ValueTypeVector:   valueP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, valueMayBeCompacted, &details); valueP->name = (char*) "value"; break;
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
                  orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified date", details);
                  return NULL;
                }

                nodeP = kjString(orionldState.kjsonP, "observedAt", date);
              }
              else
                nodeP = kjFloat(orionldState.kjsonP, mdName, mdP->numberValue);
              break;

            case orion::ValueTypeString:
              if (valueMayBeCompacted == true)
              {
                char* compactedValue = orionldContextItemAliasLookup(orionldState.contextP, mdP->stringValue.c_str(), NULL, NULL);

                if (compactedValue != NULL)
                  nodeP = kjString(orionldState.kjsonP, mdName, compactedValue);
                else
                  nodeP = kjString(orionldState.kjsonP, mdName, mdP->stringValue.c_str());
             }
              else
                nodeP = kjString(orionldState.kjsonP, mdName, mdP->stringValue.c_str());
              break;

            case orion::ValueTypeBoolean:  nodeP = kjBoolean(orionldState.kjsonP, mdName, mdP->boolValue);                     break;
            case orion::ValueTypeNull:     nodeP = kjNull(orionldState.kjsonP, mdName);                                        break;
            case orion::ValueTypeNotGiven: nodeP = kjString(orionldState.kjsonP, mdName, "UNKNOWN TYPE IN MONGODB 2");         break;

            case orion::ValueTypeObject:   nodeP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, valueMayBeCompacted, &details);  nodeP->name = (char*) "value"; break;
            case orion::ValueTypeVector:   nodeP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, valueMayBeCompacted, &details);  nodeP->name = (char*) "value"; break;
            }
          }

          if (nodeP == NULL)
            LM_E(("Error in creation of KjNode for metadata (%s)", (details != NULL)? details : "no details"));
          else
            kjChildAdd(aTop, nodeP);
        }
      }
    }
  }

  return root;
}
