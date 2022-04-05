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
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/payloadCheck/pcheckReceiverInfo.h"            // Own interface



// -----------------------------------------------------------------------------
//
// pcheckReceiverInfo -
//
bool pcheckReceiverInfo(KjNode* riP)
{
  for (KjNode* riItemP = riP->value.firstChildP; riItemP != NULL; riItemP = riItemP->next)
  {
    OBJECT_CHECK(riItemP, "Subscription::notification::endpoint::receiverInfo item");

    KjNode* keyP   = NULL;
    KjNode* valueP = NULL;

    for (KjNode* nodeP = riItemP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (strcmp(nodeP->name, "key") == 0)
      {
        DUPLICATE_CHECK(keyP, "endpoint::receiverInfo::key", nodeP);
        STRING_CHECK(keyP, "endpoint::receiverInfo::key");
      }
      else if (strcmp(nodeP->name, "value") == 0)
      {
        DUPLICATE_CHECK(valueP, "endpoint::receiverInfo::value", nodeP);
        STRING_CHECK(valueP, "endpoint::receiverInfo::value");
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid field for 'endpoint::receiverInfo'", nodeP->name, 400);
        return false;
      }
    }

    if (keyP == NULL)
    {
      orionldError(OrionldBadRequestData, "Mandatory field missing in endpoint::receiverInfo array item", "key", 400);
      return false;
    }

    if (valueP == NULL)
    {
      orionldError(OrionldBadRequestData, "Mandatory field missing in endpoint::receiverInfo array item", "value", 400);
      return false;
    }
  }

  return true;
}
