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
#include <string.h>                                             // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
}

#include "orionld/payloadCheck/PCHECK.h"                        // PCHECK_*
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/payloadCheck/pCheckRegistrationManagement.h"  // Own interface



// -----------------------------------------------------------------------------
//
// pCheckRegistrationManagement -
//
bool pCheckRegistrationManagement(KjNode* rmP)
{
  KjNode* localOnlyP = NULL;
  KjNode* timeoutP   = NULL;
  KjNode* cooldownP  = NULL;

  for (KjNode* rmItemP = rmP->value.firstChildP; rmItemP != NULL; rmItemP = rmItemP->next)
  {
    if (strcmp(rmItemP->name, "localOnly") == 0)
    {
      PCHECK_DUPLICATE(localOnlyP, rmItemP, 0, NULL, "Registration::management::localOnly", 400);
      PCHECK_BOOL(localOnlyP, 0, NULL, "Registration::management::localOnly", 400);
    }
    else if (strcmp(rmItemP->name, "timeout") == 0)
    {
      PCHECK_DUPLICATE(timeoutP, rmItemP, 0, NULL, "Registration::management::timeout", 400);
      PCHECK_NUMBER(timeoutP, 0, NULL, "Registration::management::timeout", 400);

      if (timeoutP->type == KjFloat)
      {
        if (timeoutP->value.f <= 0.001)
        {
          orionldError(OrionldBadRequestData, "Non-supported value for Registration::management::timeout", "Must be Greater Than 0", 400);
          return false;
        }
      }
      else if (timeoutP->type == KjInt)
      {
        if (timeoutP->value.i <= 0)
        {
          orionldError(OrionldBadRequestData, "Non-supported value for Registration::management::timeout", "Must be Greater Than 0", 400);
          return false;
        }
      }
    }
    else if (strcmp(rmItemP->name, "cooldown") == 0)
    {
      PCHECK_DUPLICATE(cooldownP, rmItemP, 0, NULL, "Registration::management::cooldown", 400);
      PCHECK_NUMBER(cooldownP, 0, NULL, "Registration::management::cooldown", 400);

      if (cooldownP->type == KjFloat)
      {
        if (cooldownP->value.f <= 0.001)
        {
          orionldError(OrionldBadRequestData, "Non-supported value for Registration::management::cooldown", "Must be Greater Than 0", 400);
          return false;
        }
      }
      else if (cooldownP->type == KjInt)
      {
        if (cooldownP->value.i <= 0)
        {
          orionldError(OrionldBadRequestData, "Non-supported value for Registration::management::cooldown", "Must be Greater Than 0", 400);
          return false;
        }
      }
    }
    else if (strcmp(rmItemP->name, "cacheDuration") == 0)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", "Registration::management::cacheDuration", 501);
      return false;
    }
    else
    {
      orionldError(OrionldBadRequestData, "Unrecognized field in Registration::management", rmItemP->name, 400);
      return false;
    }
  }

  return true;
}
