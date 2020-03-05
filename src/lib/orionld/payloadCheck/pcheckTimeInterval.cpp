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
#include "kjson/KjNode.h"                                       // KjNode
}

#include "rest/ConnectionInfo.h"                                // ConnectionInfo

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/common/orionldErrorResponse.h"                // orionldErrorResponseCreate
#include "orionld/payloadCheck/pcheckTimeInterval.h"            // Own interface



// ----------------------------------------------------------------------------
//
// pcheckTimeInterval -
//
bool pcheckTimeInterval(ConnectionInfo* ciP, KjNode* timeIntervalNodeP, const char* fieldName)
{
  KjNode* startP = NULL;
  KjNode* endP   = NULL;
  int64_t start  = 0;
  int64_t end    = 0;

  for (KjNode* tiItemP = timeIntervalNodeP->value.firstChildP; tiItemP != NULL; tiItemP = tiItemP->next)
  {
    if (strcmp(tiItemP->name, "start") == 0)
    {
      DUPLICATE_CHECK(startP, "start", tiItemP);
      STRING_CHECK(tiItemP, "start");
      DATETIME_CHECK(tiItemP->value.s, start, "start");
    }
    else if (strcmp(tiItemP->name, "end") == 0)
    {
      DUPLICATE_CHECK(endP, "end", tiItemP);
      STRING_CHECK(tiItemP, "end");
      DATETIME_CHECK(tiItemP->value.s, end, "end");
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for TimeInterval", tiItemP->name);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  if ((startP == NULL) && (endP == NULL))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Empty Object", fieldName);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (startP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing mandatory field", "start");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (endP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing mandatory field", "end");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (start > end)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Inconsistent TimeInterval", "TimeInterval ends before it starts");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}
