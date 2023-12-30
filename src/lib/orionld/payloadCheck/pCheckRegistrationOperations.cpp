/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <unistd.h>                                             // NULL

extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
}

#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/types/DistOpType.h"                           // distOpTypeFromString, distOpTypeAliasFromString, DoNone
#include "orionld/payloadCheck/pCheckRegistrationOperations.h"  // Own interface



// -----------------------------------------------------------------------------
//
// pCheckRegistrationOperations -
//
bool pCheckRegistrationOperations(KjNode* operationsP)
{
  for (KjNode* fwdOpP = operationsP->value.firstChildP; fwdOpP != NULL; fwdOpP = fwdOpP->next)
  {
    if (fwdOpP->type != KjString)
    {
      orionldError(OrionldBadRequestData, "Invalid JSON type for Registration::operations array item (not a JSON String)", kjValueType(fwdOpP->type), 400);
      return false;
    }

    if (fwdOpP->value.s[0] == 0)
    {
      orionldError(OrionldBadRequestData, "Empty String in Registration::operations array", "", 400);
      return false;
    }

    if (distOpTypeFromString(fwdOpP->value.s) == DoNone)
    {
      if (distOpTypeAliasFromString(fwdOpP->value.s) == DoNone)
      {
        orionldError(OrionldBadRequestData, "Invalid value for Registration::operations array item", fwdOpP->value.s, 400);
        return false;
      }
    }
  }

  return true;
}
