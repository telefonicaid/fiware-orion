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
extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
}

#include "logMsg/logMsg.h"                                      // LM_*

#include "orionld/common/CHECK.h"                               // OBJECT_CHECK, DUPLICATE_CHECK, STRING_CHECK, ...
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/payloadCheck/pcheckNotifierInfo.h"            // Own interface



// -----------------------------------------------------------------------------
//
// pcheckNotifierInfo -
//
bool pcheckNotifierInfo(KjNode* niP)
{
  for (KjNode* niItemP = niP->value.firstChildP; niItemP != NULL; niItemP = niItemP->next)
  {
    OBJECT_CHECK(niItemP, "Subscription::notification::endpoint::notifierInfo item");

    KjNode* keyP   = NULL;
    KjNode* valueP = NULL;

    for (KjNode* nodeP = niItemP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (strcmp(nodeP->name, "key") == 0)
      {
        DUPLICATE_CHECK(keyP, "endpoint::notifierInfo::key", nodeP);
        STRING_CHECK(keyP, "endpoint::notifierInfo::key");
      }
      else if (strcmp(nodeP->name, "value") == 0)
      {
        DUPLICATE_CHECK(valueP, "endpoint::notifierInfo::value", nodeP);
        STRING_CHECK(valueP, "endpoint::notifierInfo::value");
      }
      else
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for 'endpoint::notifierInfo'", nodeP->name);
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }

    if (keyP == NULL)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Mandatory field missing in endpoint::notifierInfo array item", "key");
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }

    if (valueP == NULL)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Mandatory field missing in endpoint::notifierInfo array item", "value");
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
  }

  return true;
}
