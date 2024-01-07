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
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "ngsi/ContextAttribute.h"                             // ContextAttribute

#include "orionld/types/OrionldContext.h"                      // OrionldContext
#include "orionld/types/OrionldRenderFormat.h"                 // OrionldRenderFormat
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/apiModel/langFixNormalized.h"                // langFixNormalized
#include "orionld/apiModel/langFixSimplified.h"                // langFixSimplified
#include "orionld/legacyDriver/kjTreeFromCompoundValue.h"      // kjTreeFromCompoundValue
#include "orionld/legacyDriver/kjTreeFromContextAttribute.h"   // Own interface



// -----------------------------------------------------------------------------
//
// ALLOCATION_CHECK -
//
#define ALLOCATION_CHECK(nodeP)                \
  if (nodeP == NULL)                           \
  {                                            \
    *detailsP = (char*) "unable to allocate";  \
    return NULL;                               \
  }



// -----------------------------------------------------------------------------
//
// kjTreeFromContextAttribute -
//
KjNode* kjTreeFromContextAttribute(ContextAttribute* caP, OrionldContext* contextP, OrionldRenderFormat renderFormat, const char* lang, char** detailsP)
{
  char*    attrName  = (char*) caP->name.c_str();
  KjNode*  nodeP     = NULL;

  //
  // We want shortnames unless it's Cross API and not COMPACT
  //
  if ((renderFormat != RF_CROSS_APIS_NORMALIZED) && (renderFormat != RF_CROSS_APIS_SIMPLIFIED))
  {
    char* alias = orionldContextItemAliasLookup(contextP, attrName, NULL, NULL);

    if (alias != NULL)
      attrName = alias;
  }


  //
  // keyValues?
  //
  if ((renderFormat == RF_SIMPLIFIED) || (renderFormat == RF_CROSS_APIS_SIMPLIFIED) || (renderFormat == RF_CROSS_APIS_SIMPLIFIED_COMPACT))
  {
    //
    // FIXME: This almost identical switch is in many places. Time to unite ...
    //        KjNode* kjTreeValue(ContextAttribute* aP, const char* fieldName) ?
    //
    switch (caP->valueType)
    {
    case orion::ValueTypeString:
      nodeP = kjString(orionldState.kjsonP, attrName, caP->stringValue.c_str());
      ALLOCATION_CHECK(nodeP);
      break;

    case orion::ValueTypeNumber:
      nodeP = kjFloat(orionldState.kjsonP, attrName, caP->numberValue);  // FIXME: kjInteger or kjFloat ...
      ALLOCATION_CHECK(nodeP);
      break;

    case orion::ValueTypeBoolean:
      nodeP = kjBoolean(orionldState.kjsonP, attrName, (KBool) caP->boolValue);
      ALLOCATION_CHECK(nodeP);
      break;

    case orion::ValueTypeNull:
      nodeP = kjNull(orionldState.kjsonP, attrName);
      ALLOCATION_CHECK(nodeP);
      break;

    case orion::ValueTypeVector:
    case orion::ValueTypeObject:
      nodeP = kjTreeFromCompoundValue(caP->compoundValueP, NULL, false, detailsP);
      if (nodeP == NULL)
        return NULL;
      nodeP->name = attrName;
      if ((lang != NULL) && (lang[0] != 0) && (strcmp(caP->type.c_str(), "LanguageProperty") == 0))
        langFixSimplified(nodeP, lang);
      break;

    case orion::ValueTypeNotGiven:
      nodeP = kjString(orionldState.kjsonP, attrName, "UNKNOWN TYPE");
      ALLOCATION_CHECK(nodeP);
      break;
    }

    return nodeP;
  }


  //
  // Normalized
  //
  KjNode* aTopNodeP = kjObject(orionldState.kjsonP, attrName);  // Top node for the attribute

  if (aTopNodeP == NULL)
  {
    *detailsP = (char*) "unable to allocate memory";
    return NULL;
  }

  bool     isRelationship = false;
  bool     isLanguageMap  = false;
  KjNode*  typeNodeP      = NULL;

  if (caP->type != "")
  {
    typeNodeP = kjString(orionldState.kjsonP, "type", caP->type.c_str());

    if (typeNodeP == NULL)
    {
      *detailsP = (char*) "unable to create KjString node for attribute type";
      return NULL;
    }

    kjChildAdd(aTopNodeP, typeNodeP);

    if (strcmp(typeNodeP->value.s, "Relationship") == 0)
      isRelationship = true;
    else if (strcmp(typeNodeP->value.s, "LanguageProperty") == 0)
      isLanguageMap = true;
  }

  // Value
  char* valueName = (char*) "value";

  if (isRelationship == true)
    valueName = (char*) "object";
  else if (isLanguageMap  == true)
    valueName = (char*) "languageMap";

  switch (caP->valueType)
  {
  case orion::ValueTypeString:
    nodeP = kjString(orionldState.kjsonP, valueName, caP->stringValue.c_str());
    ALLOCATION_CHECK(nodeP);
    break;

  case orion::ValueTypeNumber:
    nodeP = kjFloat(orionldState.kjsonP, "value", caP->numberValue);  // FIXME: kjInteger or kjFloat ...
    ALLOCATION_CHECK(nodeP);
    break;

  case orion::ValueTypeBoolean:
    nodeP = kjBoolean(orionldState.kjsonP, "value", (KBool) caP->boolValue);
    ALLOCATION_CHECK(nodeP);
    break;

  case orion::ValueTypeNull:
    nodeP = kjNull(orionldState.kjsonP, "value");
    ALLOCATION_CHECK(nodeP);
    break;

  case orion::ValueTypeVector:
  case orion::ValueTypeObject:
    nodeP = kjTreeFromCompoundValue(caP->compoundValueP, NULL, false, detailsP);

    if (nodeP == NULL)
      return NULL;

    nodeP->name = valueName;
    if (isLanguageMap == true)
    {
      if ((lang != NULL) && (lang[0] != 0))
        langFixNormalized(aTopNodeP, typeNodeP, nodeP, lang);
    }
    break;

  case orion::ValueTypeNotGiven:
    nodeP = kjString(orionldState.kjsonP, "value", "UNKNOWN TYPE");
    ALLOCATION_CHECK(nodeP);
    break;
  }

  kjChildAdd(aTopNodeP, nodeP);

  bool isGeoProperty = false;
  if (strcmp(caP->type.c_str(), "GeoProperty") == 0)
  {
    //
    // GeoProperty attributes seem to get an extra metadata, called "location". It needs to be removed
    //
    isGeoProperty = true;
  }

  //
  // Metadata
  //   If NGSIv2 rendering, the metadatas are put inside a "metadata" object.
  //   Empty of not, it is always rendered
  //
  KjNode* mdArrayNodeP = NULL;
  if ((renderFormat == RF_CROSS_APIS_NORMALIZED) || (renderFormat == RF_CROSS_APIS_NORMALIZED_COMPACT))
  {
    mdArrayNodeP = kjObject(orionldState.kjsonP, "metadata");
    kjChildAdd(aTopNodeP, mdArrayNodeP);
  }

  for (unsigned int ix = 0; ix < caP->metadataVector.size(); ix++)
  {
    Metadata*   mdP    = caP->metadataVector[ix];
    const char* mdName = mdP->name.c_str();

    if ((isGeoProperty == true) && (strcmp(mdName, "location") == 0))
    {
      nodeP = kjObject(orionldState.kjsonP, mdName);

      KjNode* typeP  = kjString(orionldState.kjsonP, "type", "GeoProperty");
      KjNode* valueP = kjObject(orionldState.kjsonP, "value");

      kjChildAdd(nodeP, typeP);
      kjChildAdd(nodeP, valueP);
    }

    //
    // Special cases:
    //
    // * observedAt - stored as Number but must be served as a string ...
    //                also, not expanded
    // * unitCode - presented as key-value
    //
    if (strcmp(mdName, "observedAt") == 0)
    {
      char     date[128];

      if (numberToDate((time_t) mdP->numberValue, date, sizeof(date)) == false)
      {
        LM_E(("numberToDate failed"));
        return NULL;
      }

      nodeP = kjString(orionldState.kjsonP, mdName, date);
    }
    else if (strcmp(mdName, "unitCode") == 0)
      nodeP = kjString(orionldState.kjsonP, mdName, mdP->stringValue.c_str());
    else
    {
      char*   mdLongName     = orionldContextItemAliasLookup(contextP, mdName, NULL, NULL);
      KjNode* typeNodeP      = kjString(orionldState.kjsonP, "type", mdP->type.c_str());
      KjNode* valueNodeP     = NULL;

      nodeP = kjObject(orionldState.kjsonP, mdLongName);

      kjChildAdd(nodeP, typeNodeP);

      if (strcmp(mdP->type.c_str(), "Relationship") == 0)
      {
        const char* valueName = (renderFormat < RF_CROSS_APIS_NORMALIZED)? "object" : "value";  // Not Cross API: "object"

        valueNodeP = kjString(orionldState.kjsonP, valueName, mdP->stringValue.c_str());
      }
      else if (strcmp(mdP->type.c_str(), "Property") == 0)
      {
        switch (mdP->valueType)
        {
        case orion::ValueTypeString:
          valueNodeP = kjString(orionldState.kjsonP, "value", mdP->stringValue.c_str());
          break;

        case orion::ValueTypeNumber:
          valueNodeP = kjFloat(orionldState.kjsonP, "value", mdP->numberValue);  // FIXME: kjInteger or kjFloat
          break;

        case orion::ValueTypeBoolean:
          valueNodeP = kjBoolean(orionldState.kjsonP, "value", (KBool) mdP->boolValue);
          break;

        case orion::ValueTypeNull:
          valueNodeP = kjNull(orionldState.kjsonP, "value");
          break;

        case orion::ValueTypeVector:
        case orion::ValueTypeObject:
          valueNodeP = kjTreeFromCompoundValue(mdP->compoundValueP, NULL, false, detailsP);
          if (valueNodeP != NULL)
            valueNodeP->name = (char*) "value";
          break;

        case orion::ValueTypeNotGiven:
          valueNodeP = kjString(orionldState.kjsonP, "value", "UNKNOWN TYPE");
          break;
        }
      }
      else
      {
        valueNodeP = kjString(orionldState.kjsonP, "NonSupportedAttributeType", mdP->type.c_str());
      }

      kjChildAdd(nodeP, valueNodeP);
    }

    if (mdArrayNodeP != NULL)
      kjChildAdd(mdArrayNodeP, nodeP);
    else
      kjChildAdd(aTopNodeP, nodeP);
  }

  return aTopNodeP;
}
