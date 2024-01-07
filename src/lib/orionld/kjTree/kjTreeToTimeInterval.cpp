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
#include <vector>                                              // std::vector

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/dateTime.h"                           // dateTimeFromString
#include "orionld/types/OrionldTimeInterval.h"                 // OrionldTimeInterval



// -----------------------------------------------------------------------------
//
// kjTreeToTimeInterval -
//
bool kjTreeToTimeInterval(KjNode* kNodeP, OrionldTimeInterval* intervalP)
{
  KjNode* startP = NULL;
  KjNode* endP   = NULL;

  for (KjNode* intervalItemP = kNodeP->value.firstChildP; intervalItemP != NULL; intervalItemP = intervalItemP->next)
  {
    if (strcmp(intervalItemP->name, "startAt") == 0)
    {
      DUPLICATE_CHECK(startP, "TimeInterval::startAt", intervalItemP);
      STRING_CHECK(startP, "TimeInterval::startAt");
    }
    else if (strcmp(intervalItemP->name, "endAt") == 0)
    {
      DUPLICATE_CHECK(endP, "TimeInterval::end", intervalItemP);
      STRING_CHECK(endP, "TimeInterval::end");
    }
    else
    {
      orionldError(OrionldBadRequestData, "Unexpected field in TimeInterval", intervalItemP->name, 400);
      return false;
    }
  }

  if ((startP == NULL) || (endP == NULL))
  {
    const char* missing = (startP == NULL)? "TimeInterval::start" : "TimeInterval::end";
    orionldError(OrionldBadRequestData, "Missing field in TimeInterval", missing, 400);
    return false;
  }

  char errorString[256];
  if ((intervalP->start = dateTimeFromString(startP->value.s, errorString, sizeof(errorString))) < 0)
  {
    orionldError(OrionldBadRequestData, "Invalid ISO8601 time string", errorString, 400);
    return false;
  }

  if ((intervalP->end = dateTimeFromString(endP->value.s, errorString, sizeof(errorString))) < 0)
  {
    orionldError(OrionldBadRequestData, "Invalid ISO8601 time string", errorString, 400);
    return false;
  }

  return true;
}
