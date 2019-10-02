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
#include "kjson/kjFree.h"                                      // kjFree
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
}

#include "parse/CompoundValueNode.h"                           // CompoundValueNode
#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/context/orionldAliasLookup.h"                // orionldAliasLookup
#include "orionld/kjTree/kjTreeFromCompoundValue.h"            // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromCompoundValue2
//
static KjNode* kjTreeFromCompoundValue2(KjNode* parentP, orion::CompoundValueNode* compoundP, bool valueMayBeContracted, char** detailsP)
{
  KjNode*       nodeP = NULL;
  char*         name  = (char*) compoundP->name.c_str();
  unsigned int  size;
  char*         contractedValue;

  switch (compoundP->valueType)
  {
  case orion::ValueTypeString:
    if ((valueMayBeContracted == true) && ((contractedValue = orionldAliasLookup(orionldState.contextP, compoundP->stringValue.c_str(), NULL)) != NULL))
      nodeP = kjString(orionldState.kjsonP, name, contractedValue);
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
      KjNode* itemP = kjTreeFromCompoundValue2(nodeP, compoundP->childV[ix], valueMayBeContracted, detailsP);

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
KjNode* kjTreeFromCompoundValue(orion::CompoundValueNode* compoundP, KjNode* containerP, bool valueMayBeContracted, char** detailsP)
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
    KjNode* nodeP = kjTreeFromCompoundValue2(topNodeP, compoundP->childV[ix], valueMayBeContracted, detailsP);
    if (nodeP == NULL)
    {
      kjFree(topNodeP);
      return NULL;
    }

    kjChildAdd(topNodeP, nodeP);
  }

  return topNodeP;
}
