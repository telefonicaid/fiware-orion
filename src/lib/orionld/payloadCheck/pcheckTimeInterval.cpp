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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/CHECK.h"                                // STRING_CHECK, ...
#include "orionld/payloadCheck/pcheckTimeInterval.h"             // Own interface



// ----------------------------------------------------------------------------
//
// pcheckTimeInterval -
//
bool pcheckTimeInterval(KjNode* timeIntervalNodeP, const char* fieldName)
{
  KjNode* startAtP = NULL;
  KjNode* endAtP   = NULL;
  int64_t startAt  = 0;
  int64_t endAt    = 0;

  for (KjNode* tiItemP = timeIntervalNodeP->value.firstChildP; tiItemP != NULL; tiItemP = tiItemP->next)
  {
    if (strcmp(tiItemP->name, "startAt") == 0)
    {
      DUPLICATE_CHECK(startAtP, "startAt", tiItemP);
      STRING_CHECK(tiItemP, "startAt");
      EMPTY_STRING_CHECK(tiItemP, "startAt");
      DATETIME_CHECK(tiItemP->value.s, startAt, "startAt");
    }
    else if (strcmp(tiItemP->name, "endAt") == 0)
    {
      DUPLICATE_CHECK(endAtP, "endAt", tiItemP);
      STRING_CHECK(tiItemP, "endAt");
      EMPTY_STRING_CHECK(tiItemP, "endAt");
      DATETIME_CHECK(tiItemP->value.s, endAt, "endAt");
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid field for TimeInterval", tiItemP->name, 400);
      return false;
    }
  }

  if ((startAtP == NULL) && (endAtP == NULL))
  {
    orionldError(OrionldBadRequestData, "Empty Object", fieldName, 400);
    return false;
  }

  if (startAtP == NULL)
  {
    orionldError(OrionldBadRequestData, "Missing mandatory field", "startAt", 400);
    return false;
  }

  if (endAtP == NULL)
  {
    orionldError(OrionldBadRequestData, "Missing mandatory field", "endAt", 400);
    return false;
  }

  if (startAt > endAt)
  {
    orionldError(OrionldBadRequestData, "Inconsistent TimeInterval (it ends before it starts)", fieldName, 400);
    return false;
  }

  return true;
}
