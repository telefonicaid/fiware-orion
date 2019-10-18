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
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
#include "kjson/kjFree.h"                                      // kjFree
}

#include "parseArgs/baStd.h"                                   // BA_FT - for debugging only
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*
#include "rest/ConnectionInfo.h"                               // ConnectionInfo

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/kjTree/kjTreeFromContextContextAttribute.h"  // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromContextContextAttribute -
//
// What we have in ContextAttribute::value is what we need as value of "@context" in the Context Tree.
// We must create the toplevel object and the "@context" member, and give it the value of "ContextAttribute::value".
// If ContextAttribute::value is a string, then the Context Tree will be simply a string called "@context" with the value of ContextAttribute::value.
// If ContextAttribute::value is a vector ...
// If ContextAttribute::value is an object ...
//
KjNode* kjTreeFromContextContextAttribute(ConnectionInfo* ciP, ContextAttribute* caP, char** detailsP)
{
  KjNode* topNodeP = kjObject(orionldState.kjsonP, NULL);

  if (caP->valueType == orion::ValueTypeString)
  {
    LM_T(LmtContext, ("It's a String!"));
    KjNode* stringNodeP = kjString(orionldState.kjsonP, "@context", caP->stringValue.c_str());
    LM_T(LmtContext, ("The string is: '%s'", stringNodeP->value.s));

    kjChildAdd(topNodeP, stringNodeP);
  }
  else if (caP->compoundValueP->valueType == orion::ValueTypeVector)
  {
    LM_T(LmtContext, ("It's an Array!"));
    KjNode* arrayNodeP = kjArray(orionldState.kjsonP, "@context");

    kjChildAdd(topNodeP, arrayNodeP);

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

      KjNode* itemNodeP = kjString(orionldState.kjsonP, NULL, compoundP->stringValue.c_str());
      kjChildAdd(arrayNodeP, itemNodeP);
      LM_T(LmtContext, ("Added array item '%s' to context", itemNodeP->value.s));
    }
  }
  else if (caP->compoundValueP->valueType == orion::ValueTypeObject)
  {
    LM_T(LmtContext, ("It's an Object!"));
    KjNode* objectNodeP = kjObject(orionldState.kjsonP, "@context");

    kjChildAdd(topNodeP, objectNodeP);

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

      KjNode* itemNodeP = kjString(orionldState.kjsonP, compoundP->name.c_str(), compoundP->stringValue.c_str());
      kjChildAdd(objectNodeP, itemNodeP);

      LM_T(LmtContext, ("Added object member '%s' == '%s' to context", itemNodeP->name, itemNodeP->value.s));
    }
  }
  else
  {
    kjFree(topNodeP);
    LM_E(("Error - @context attribute value must be either String, Array, or Object"));
    return NULL;
  }

  return topNodeP;
}
