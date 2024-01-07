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
#include "kalloc/kaAlloc.h"                                       // kaAlloc
#include "kalloc/kaStrdup.h"                                      // kaStrdup
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjBuilder.h"                                      // kjObject, kjString, kjBoolean, ...
#include "kjson/kjClone.h"                                        // kjClone
#include "kjson/kjLookup.h"                                       // kjLookup
}

#include "logMsg/logMsg.h"                                        // LM_*

#include "common/string.h"                                        // FT
#include "ngsi10/QueryContextResponse.h"                          // QueryContextResponse

#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/common/orionldError.h"                          // orionldError
#include "orionld/common/numberToDate.h"                          // numberToDate
#include "orionld/common/httpStatusCodeToOrionldErrorType.h"      // httpStatusCodeToOrionldErrorType
#include "orionld/common/SCOMPARE.h"                              // SCOMPAREx
#include "orionld/common/langStringExtract.h"                     // langStringExtract
#include "orionld/context/orionldCoreContext.h"                   // orionldCoreContext
#include "orionld/context/orionldContextItemAliasLookup.h"        // orionldContextItemAliasLookup
#include "orionld/kjTree/kjTreeLog.h"                             // kjTreeLog
#include "orionld/legacyDriver/kjTreeFromContextAttribute.h"      // kjTreeFromContextAttribute
#include "orionld/legacyDriver/kjTreeFromCompoundValue.h"         // kjTreeFromCompoundValue
#include "orionld/legacyDriver/kjTreeFromQueryContextResponse.h"  // Own interface



// -----------------------------------------------------------------------------
//
// inAttrList -
//
static bool inAttrList(const char* attrName, char** attrList, int attrsInList)
{
  for (int ix = 0; ix < attrsInList; ix++)
  {
    if (strcmp(attrName, attrList[ix]) == 0)
      return true;
  }

  return false;
}


// -----------------------------------------------------------------------------
//
// orionldSysAttrs -
//
bool orionldSysAttrs(double creDate, double modDate, KjNode* containerP)
{
  char     date[128];
  KjNode*  nodeP;

  // createdAt
  if (numberToDate(creDate, date, sizeof(date)) == false)
  {
    orionldError(OrionldInternalError, "Unable to create a stringified createdAt date", NULL, 500);
    return false;
  }

  nodeP = kjString(orionldState.kjsonP, "createdAt", date);
  kjChildAdd(containerP, nodeP);

  // modifiedAt
  if (numberToDate(modDate, date, sizeof(date)) == false)
  {
    orionldError(OrionldInternalError, "Unable to create a stringified modifiedAt date", NULL, 500);
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
KjNode* kjTreeFromQueryContextResponse(bool oneHit, bool keyValues, bool concise, const char* lang, QueryContextResponse* responseP)
{
  char* details  = NULL;
  bool  sysAttrs = orionldState.uriParamOptions.sysAttrs;

  //
  // No hits when "oneHit == false" is not an error.
  // We just return an empty array
  //
  if ((oneHit == false) && (responseP->contextElementResponseVector.size() == 0))
  {
    orionldState.responseTree   = kjArray(orionldState.kjsonP, NULL);
    orionldState.httpStatusCode = 200;

    return orionldState.responseTree;
  }


  //
  // Error?
  //
  if (responseP->errorCode.code == 0)
    responseP->errorCode.code = SccOk;

  if (responseP->errorCode.code != 200)
  {
    LM_E(("Error %d from mongoBackend", responseP->errorCode.code));
    OrionldResponseErrorType errorType = httpStatusCodeToOrionldErrorType(responseP->errorCode.code);

    orionldError(errorType, responseP->errorCode.reasonPhrase.c_str(), responseP->errorCode.details.c_str(), 400);

    if (responseP->errorCode.code == 404)
      orionldState.httpStatusCode = 404;

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
    orionldError(OrionldInternalError, "More than one hit", orionldState.wildcard[0], 500);
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

  for (int ix = 0; ix < hits; ix++)
  {
    ContextElement*  ceP = &responseP->contextElementResponseVector[ix]->contextElement;
    KjNode*          nodeP;

    //
    // Creating the KjNode tree for the entity - 'top'
    // If more than one hit, 'top' is added to the array 'root'
    // If just one hit, 'top' points directly to root, whioch is then not an array but an Object
    //
    if (oneHit == false)
    {
      top = kjObject(orionldState.kjsonP, NULL);
      kjChildAdd(root, top);
    }


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
        orionldError(OrionldInternalError, "Unable to create tree node", "out of memory", 500);
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
      bool              valueMayBeCompacted  = false;

      attrName = orionldContextItemAliasLookup(orionldState.contextP, attrLongName, &valueMayBeCompacted, NULL);

      //
      // If URI param 'attrs' has been used, only matching attributes should be included in the response
      //
      if (orionldState.in.attrList.items > 0)
      {
        if (inAttrList(attrLongName, orionldState.in.attrList.array, orionldState.in.attrList.items) == false)
          continue;
      }

      if (keyValues)
      {
        if ((aP->type == "LanguageProperty") && (lang != NULL) && (aP->valueType == orion::ValueTypeObject))
        {
          KjNode* langValueP = NULL;
          char*   details    = NULL;

          langValueP = kjTreeFromCompoundValue(aP->compoundValueP, langValueP, valueMayBeCompacted, &details);
          if (langValueP == NULL)
          {
            LM_E(("kjTreeFromCompoundValue: %s", details));
            orionldError(OrionldInternalError, "Unable to create tree node from a compound value", details, 500);
            return NULL;
          }

          char*   pickedLanguage;
          KjNode* langValueNodeP = langItemPick(langValueP, attrName, lang, &pickedLanguage);

          if (langValueNodeP->type == KjString)
            aTop = kjString(orionldState.kjsonP, attrName, langValueNodeP->value.s);
          else  // langValueP->type == KjArray
            aTop = langValueNodeP;
        }
        else
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
                orionldError(OrionldInternalError, "Unable to create tree node from a compound value", details, 500);
                return NULL;
              }
            }
            break;

          case orion::ValueTypeNotGiven:
            orionldError(OrionldInternalError, "Invalid internal JSON type for Context Atribute", NULL, 500);
            break;
          }
        }

        if (aTop == NULL)
        {
          LM_E(("Internal Error (Out of memory)"));
          orionldError(OrionldInternalError, "Unable to create tree node", "out of memory", 500);
          return NULL;
        }

        kjChildAdd(top, aTop);    // Adding the attribute to the tree
      }
      else  // Normalized   AND    Concise  - concise is dealt with later!
      {
        //
        // NOT keyValues - create entire attribute tree
        //
        aTop = kjObject(orionldState.kjsonP, attrName);
        if (aTop == NULL)
        {
          LM_E(("Error creating a KjNode Object"));
          orionldError(OrionldInternalError, "Unable to create tree node", "out of memory", 500);
          return NULL;
        }

        // type
        if (aP->type != "")
        {
          nodeP = kjString(orionldState.kjsonP, "type", aP->type.c_str());
          if (nodeP == NULL)
          {
            LM_E(("Error creating a KjNode String"));
            orionldError(OrionldInternalError, "Unable to create tree node", "out of memory", 500);
            return NULL;
          }

          kjChildAdd(aTop, nodeP);
        }

        // value
        char* valueFieldName = (char*) "value";

        if      (aP->type == "Relationship")      valueFieldName = (char*) "object";
        else if (aP->type == "LanguageProperty")  valueFieldName = (char*) "languageMap";


        switch (aP->valueType)
        {
        case orion::ValueTypeNumber:
          if (SCOMPARE11(attrLongName, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
          {
            char   date[128];

            if (numberToDate(aP->numberValue, date, sizeof(date)) == false)
            {
              LM_E(("Error creating a stringified date"));
              orionldError(OrionldInternalError, "Unable to create a stringified observedAt date", NULL, 500);
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
            orionldError(OrionldInternalError, "Unable to create tree node for compound value", "out of memory", 500);
            return NULL;
          }

          if (kjTreeFromCompoundValue(aP->compoundValueP, nodeP, valueMayBeCompacted, &details) == NULL)
          {
            LM_E(("kjTreeFromCompoundValue: %s", details));
            orionldError(OrionldInternalError, "Unable to create tree node from compound value", details, 500);
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
              (strcmp(mdName, "modifiedAt") != 0) &&
              (strcmp(mdName, "unitCode")   != 0))
          {
            //
            // Looking up short name for the sub-attribute
            //
            mdName = orionldContextItemAliasLookup(orionldState.contextP, mdName, &valueMayBeCompacted, NULL);
          }

          if (mdP->type != "")
          {
            char* valueFieldName = (char*) "value";
            if      (mdP->type == "Relationship")      valueFieldName = (char*) "object";
            else if (mdP->type == "LanguageProperty")  valueFieldName = (char*) "languageMap";

            KjNode*      typeP;
            KjNode*      valueP = NULL;

            nodeP = kjObject(orionldState.kjsonP, mdName);

            typeP = kjString(orionldState.kjsonP, "type", mdP->type.c_str());
            kjChildAdd(nodeP, typeP);

            // System Attributes?
            if (sysAttrs == true)
            {
              if (orionldSysAttrs(mdP->createdAt, mdP->modifiedAt, nodeP) == false)
              {
                LM_E(("sysAttrs error"));
                return NULL;
              }
            }

            details = NULL;
            switch (mdP->valueType)
            {
            case orion::ValueTypeNumber:
              if (SCOMPARE11(mdName, 'o', 'b', 's', 'e', 'r', 'v', 'e', 'd', 'A', 't', 0))
              {
                char   date[128];

                if (numberToDate(mdP->numberValue, date, sizeof(date)) == false)
                {
                  LM_E(("Error creating a stringified date"));
                  orionldError(OrionldInternalError, "Unable to create a stringified observedAt date", NULL, 500);
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

                if (numberToDate(mdP->numberValue, date, sizeof(date)) == false)
                {
                  LM_E(("Error creating a stringified date"));
                  orionldError(OrionldInternalError, "Unable to create a stringified date", NULL, 500);
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
