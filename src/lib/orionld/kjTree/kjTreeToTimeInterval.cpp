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

#include "common/globals.h"                                    // parse8601Time
#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
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
    if (SCOMPARE6(intervalItemP->name, 's', 't', 'a', 'r', 't', 0))
    {
      DUPLICATE_CHECK(startP, "TimeInterval::start", intervalItemP);
      STRING_CHECK(startP, "TimeInterval::start");
    }
    else if (SCOMPARE4(intervalItemP->name, 'e', 'n', 'd', 0))
    {
      DUPLICATE_CHECK(endP, "TimeInterval::end", intervalItemP);
      STRING_CHECK(endP, "TimeInterval::end");
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Unexpected field in TimeInterval", intervalItemP->name);
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
  }

  if ((startP == NULL) || (endP == NULL))
  {
    const char* missing = (startP == NULL)? "TimeInterval::start" : "TimeInterval::end";
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing field in TimeInterval", missing);
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  if ((intervalP->start = parse8601Time(startP->value.s)) == -1)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid ISO8601 time string", startP->value.s);
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  if ((intervalP->end = parse8601Time(endP->value.s)) == -1)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid ISO8601 time string", endP->value.s);
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}
