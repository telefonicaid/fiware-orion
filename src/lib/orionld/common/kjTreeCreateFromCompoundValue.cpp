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

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "parse/CompoundValueNode.h"                           // CompoundValueNode
#include "orionld/common/kjTreeCreateFromCompoundValue.h"      // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeCreateFromCompoundValue2
//
static KjNode* kjTreeCreateFromCompoundValue2(ConnectionInfo* ciP, KjNode* parentP, orion::CompoundValueNode* compoundP, char** detailsP)
{
  KjNode*       nodeP;
  char*         name = (char*) compoundP->name.c_str();
  unsigned int  size;

  switch (compoundP->valueType)
  {
  case orion::ValueTypeString:
    nodeP = kjString(ciP->kjsonP, name, compoundP->stringValue.c_str());
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeNumber:
    nodeP = kjFloat(ciP->kjsonP, name, compoundP->numberValue);  // FIXME: kjInteger or kjFloat ...
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeBoolean:
    nodeP = kjBoolean(ciP->kjsonP, name, (KBool) compoundP->boolValue);
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeNull:
    nodeP = kjNull(ciP->kjsonP, name);
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeObject:
  case orion::ValueTypeVector:
    if (compoundP->valueType == orion::ValueTypeVector)
      nodeP = kjArray(ciP->kjsonP, name);
    else
      nodeP = kjObject(ciP->kjsonP, name);

    size = compoundP->childV.size();
    for (unsigned int ix = 0; ix < size; ++ix)
    {
      KjNode* itemP = kjTreeCreateFromCompoundValue2(ciP, nodeP,  compoundP->childV[ix], detailsP);

      if (itemP == NULL)
        return NULL;

      kjChildAdd(nodeP, itemP);
    }
    
    kjChildAdd(parentP, nodeP);
    break;

  case orion::ValueTypeNotGiven:
    nodeP = kjString(ciP->kjsonP, name, "UNKNOWN TYPE");
    kjChildAdd(parentP, nodeP);
    break;
  }

  return nodeP;
}



// -----------------------------------------------------------------------------
//
// kjTreeCreateFromCompoundValue -
//
KjNode* kjTreeCreateFromCompoundValue(ConnectionInfo* ciP, orion::CompoundValueNode* compoundP, char** detailsP)
{
  KjNode* topNodeP;

  if (compoundP->valueType == orion::ValueTypeObject)
    topNodeP = kjObject(ciP->kjsonP, NULL);
  else if (compoundP->valueType == orion::ValueTypeVector)
    topNodeP = kjArray(ciP->kjsonP, NULL);
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

  unsigned int size = compoundP->childV.size();
  for (unsigned int ix = 0; ix < size; ++ix)
  {
    KjNode* nodeP = kjTreeCreateFromCompoundValue2(ciP, topNodeP, compoundP->childV[ix], detailsP);
    if (nodeP == NULL)
    {
      kjFree(topNodeP);
      return NULL;
    }

    kjChildAdd(topNodeP, nodeP);
  }

  return topNodeP;
}
