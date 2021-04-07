/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "parse/CompoundValueNode.h"                             // orion::CompoundValueNode

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/types/OrionldResponseErrorType.h"              // OrionldBadRequestData, ...
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/kjTree/kjTreeToCompoundValue.h"                // Own interface



// -----------------------------------------------------------------------------
//
// compoundValueNodeValueSet - set the value of a CompoundeValueNode instance
//
static bool compoundValueNodeValueSet(orion::CompoundValueNode* cNodeP, KjNode* kNodeP, int* levelP)
{
  if (kNodeP->type == KjString)
  {
    cNodeP->valueType   = orion::ValueTypeString;
    cNodeP->stringValue = kNodeP->value.s;
  }
  else if (kNodeP->type == KjBoolean)
  {
    cNodeP->valueType   = orion::ValueTypeBoolean;
    cNodeP->boolValue   = kNodeP->value.b;
  }
  else if (kNodeP->type == KjFloat)
  {
    cNodeP->valueType   = orion::ValueTypeNumber;
    cNodeP->numberValue = kNodeP->value.f;
  }
  else if (kNodeP->type == KjInt)
  {
    cNodeP->valueType   = orion::ValueTypeNumber;
    cNodeP->numberValue = kNodeP->value.i;
  }
  else if (kNodeP->type == KjNull)
  {
    cNodeP->valueType = orion::ValueTypeNull;
  }
  else if (kNodeP->type == KjObject)
  {
    *levelP += 1;
    cNodeP->valueType = orion::ValueTypeObject;

    for (KjNode* kChildP = kNodeP->value.firstChildP; kChildP != NULL; kChildP = kChildP->next)
    {
      orion::CompoundValueNode* cChildP = kjTreeToCompoundValue(kChildP, kNodeP, *levelP);
      cNodeP->childV.push_back(cChildP);
    }
  }
  else if (kNodeP->type == KjArray)
  {
    *levelP += 1;
    cNodeP->valueType = orion::ValueTypeVector;

    for (KjNode* kChildP = kNodeP->value.firstChildP; kChildP != NULL; kChildP = kChildP->next)
    {
      orion::CompoundValueNode* cChildP = kjTreeToCompoundValue(kChildP, kNodeP, *levelP);

      cNodeP->childV.push_back(cChildP);
    }
  }
  else
  {
    LM_E(("Invalid json type (KjNone!) for value field of compound '%s'", cNodeP->name.c_str()));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal error", "Invalid type from kjson");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToCompoundValue -
//
orion::CompoundValueNode* kjTreeToCompoundValue(KjNode* kNodeP, KjNode* parentP, int level)
{
  orion::CompoundValueNode* cNodeP = new orion::CompoundValueNode();

  if ((parentP != NULL) && (parentP->type == KjObject))
    cNodeP->name = kNodeP->name;

  if (compoundValueNodeValueSet(cNodeP, kNodeP, &level) == false)
  {
    // compoundValueNodeValueSet calls orionldErrorResponseCreate
    return NULL;
  }

  return cNodeP;
}
