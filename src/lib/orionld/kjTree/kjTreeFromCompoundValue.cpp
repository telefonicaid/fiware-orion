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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjFree.h"                                      // kjFree
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "parse/CompoundValueNode.h"                           // CompoundValueNode
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/kjTree/kjTreeFromCompoundValue.h"            // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromCompoundValue2
//
static KjNode* kjTreeFromCompoundValue2(KjNode* parentP, orion::CompoundValueNode* compoundP, bool valueMayBeCompacted, char** detailsP)
{
  KjNode*       nodeP = NULL;
  char*         name  = (char*) compoundP->name.c_str();
  unsigned int  size;
  char*         compactedValue;

  switch (compoundP->valueType)
  {
  case orion::ValueTypeString:
    if ((valueMayBeCompacted == true) && ((compactedValue = orionldContextItemAliasLookup(orionldState.contextP, compoundP->stringValue.c_str(), NULL, NULL)) != NULL))
      nodeP = kjString(orionldState.kjsonP, name, compactedValue);
    else
      nodeP = kjString(orionldState.kjsonP, name, compoundP->stringValue.c_str());
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeNumber:
    nodeP = kjFloat(orionldState.kjsonP, name, compoundP->numberValue);  // FIXME: kjInteger or kjFloat ...
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeBoolean:
    nodeP = kjBoolean(orionldState.kjsonP, name, (KBool) compoundP->boolValue);
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeNull:
    nodeP = kjNull(orionldState.kjsonP, name);
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeObject:
  case orion::ValueTypeVector:
    if (compoundP->valueType == orion::ValueTypeVector)
      nodeP = kjArray(orionldState.kjsonP, name);
    else
      nodeP = kjObject(orionldState.kjsonP, name);
    size = compoundP->childV.size();
    for (unsigned int ix = 0; ix < size; ++ix)
    {
      KjNode* itemP = kjTreeFromCompoundValue2(nodeP, compoundP->childV[ix], valueMayBeCompacted, detailsP);

      if (itemP == NULL)
        return NULL;

      kjChildAdd(nodeP, itemP);
    }

    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeNotGiven:
  default:
    nodeP = kjString(orionldState.kjsonP, name, "UNKNOWN TYPE");
    kjChildAdd(parentP, nodeP);
    break;
  }

  return nodeP;
}



// -----------------------------------------------------------------------------
//
// kjTreeFromCompoundValue -
//
KjNode* kjTreeFromCompoundValue(orion::CompoundValueNode* compoundP, KjNode* containerP, bool valueMayBeCompacted, char** detailsP)
{
  KjNode* topNodeP = containerP;

  if (topNodeP == NULL)
  {
    if (compoundP->valueType == orion::ValueTypeObject)
      topNodeP = kjObject(orionldState.kjsonP, NULL);
    else if (compoundP->valueType == orion::ValueTypeVector)
      topNodeP = kjArray(orionldState.kjsonP, NULL);
    else
    {
      *detailsP = (char*) "not a compound";
      return NULL;
    }

    if (topNodeP == NULL)
    {
      *detailsP = (char*) "unable to allocate";
      return NULL;
    }
  }

  unsigned int size = compoundP->childV.size();
  for (unsigned int ix = 0; ix < size; ++ix)
  {
    KjNode* nodeP = kjTreeFromCompoundValue2(topNodeP, compoundP->childV[ix], valueMayBeCompacted, detailsP);
    if (nodeP == NULL)
    {
      kjFree(topNodeP);
      return NULL;
    }

    kjChildAdd(topNodeP, nodeP);
  }

  return topNodeP;
}
