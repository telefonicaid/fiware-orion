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
#include "ngsi/ContextAttribute.h"                             // ContextAttribute
#include "orionld/common/orionldErrorResponse.h"               // OrionldResponseErrorType, orionldErrorResponse
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext
#include "orionld/context/orionldContextValueLookup.h"         // orionldContextValueLookup
#include "orionld/context/orionldContextCreateFromTree.h"      // orionldContextCreateFromTree
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInsert
#include "orionld/kjTree/kjTreeFromCompoundValue.h"            // kjTreeFromCompoundValue
#include "orionld/kjTree/kjTreeFromContextAttribute.h"         // Own interface



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
KjNode* kjTreeFromContextAttribute(ConnectionInfo* ciP, ContextAttribute* caP, char** detailsP)
{
  KjNode* aTopNodeP = kjObject(ciP->kjsonP, caP->name.c_str());  // Top node for the attribute

  if (aTopNodeP == NULL)
  {
    *detailsP = (char*) "unable to allocate memory";
    return NULL;
  }

  if (caP->type != "")
  {
    KjNode* typeNodeP = kjString(ciP->kjsonP, "type", caP->type.c_str());

    if (typeNodeP == NULL)
    {
      *detailsP = (char*) "unable to create KjString node for attribute type";
      free(aTopNodeP);
      return NULL;
    }

    kjChildAdd(aTopNodeP, typeNodeP);
  }
  
  // This is for ngsi-ld only, so, no metadata

  // Value
  KjNode* nodeP;

  switch (caP->valueType)
  {
  case orion::ValueTypeString:
    nodeP = kjString(ciP->kjsonP, "value", caP->stringValue.c_str());
    ALLOCATION_CHECK(nodeP);
    kjChildAdd(aTopNodeP, nodeP);
    break;

  case orion::ValueTypeNumber:
    nodeP = kjFloat(ciP->kjsonP, "value", caP->numberValue);  // FIXME: kjInteger or kjFloat ...
    ALLOCATION_CHECK(nodeP);
    kjChildAdd(aTopNodeP, nodeP);
    break;

  case orion::ValueTypeBoolean:
    nodeP = kjBoolean(ciP->kjsonP, "value", (KBool) caP->boolValue);
    ALLOCATION_CHECK(nodeP);
    kjChildAdd(aTopNodeP, nodeP);
    break;

  case orion::ValueTypeNull:
    nodeP = kjNull(ciP->kjsonP, "value");
    ALLOCATION_CHECK(nodeP);
    kjChildAdd(aTopNodeP, nodeP);
    break;

  case orion::ValueTypeVector:
  case orion::ValueTypeObject:
    nodeP = kjTreeFromCompoundValue(ciP, caP->compoundValueP, detailsP);
    if (nodeP == NULL)
      return NULL;
    kjChildAdd(aTopNodeP, nodeP);
    break;

  case orion::ValueTypeNotGiven:
    nodeP = kjString(ciP->kjsonP, "value", "UNKNOWN TYPE");
    ALLOCATION_CHECK(nodeP);
    kjChildAdd(aTopNodeP, nodeP);
    break;
  }

  return aTopNodeP;
}
