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
#include <stdlib.h>                                             // strtoul

extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
}

#include "logMsg/logMsg.h"                                      // LM_*

#include "orionld/common/CHECK.h"                               // OBJECT_CHECK, DUPLICATE_CHECK, STRING_CHECK, ...
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/payloadCheck/pcheckNotifierInfo.h"            // Own interface



// -----------------------------------------------------------------------------
//
// pcheckNotifierInfo -
//
bool pcheckNotifierInfo(KjNode* niP, bool* mqttChangeP)
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
        orionldError(OrionldBadRequestData, "Invalid field for 'endpoint::notifierInfo'", nodeP->name, 400);
        return false;
      }
    }

    if (keyP == NULL)
    {
      orionldError(OrionldBadRequestData, "Mandatory field missing in endpoint::notifierInfo array item", "key", 400);
      return false;
    }

    if (valueP == NULL)
    {
      orionldError(OrionldBadRequestData, "Mandatory field missing in endpoint::notifierInfo array item", "value", 400);
      return false;
    }

    //
    // Only two "notifier infos" are supported at the moment:
    // * MQTT-Version
    // * MQTT-QoS
    // And these two support just a few values:
    // * MQTT-Version:
    //   - mqtt5.0 (default)
    //   - mqtt3.1.1
    //
    // * MQTT-QoS
    //   - 0
    //   - 1
    //   - 2
    //
    if (strcmp(keyP->value.s, "MQTT-Version") == 0)
    {
      if ((strcmp(valueP->value.s, "mqtt5.0") != 0) && (strcmp(valueP->value.s, "mqtt3.1.1") != 0))
      {
        orionldError(OrionldBadRequestData, "Invalid value for notifierInfo item /MQTT-Version/", valueP->value.s, 400);
        return false;
      }

      *mqttChangeP = true;
    }
    else if (strcmp(keyP->value.s, "MQTT-QoS") == 0)
    {
      char*        rest = NULL;
      unsigned int QoS  = strtoul(valueP->value.s, &rest, 10);

      if (*rest != 0)
      {
        orionldError(OrionldBadRequestData, "Invalid value for notifierInfo item /MQTT-OoS/ - not a valid number", valueP->value.s, 400);
        return false;
      }

      if (QoS > 2)
      {
        orionldError(OrionldBadRequestData, "Invalid value for notifierInfo item /MQTT-OoS/ - only 0, 1, and 2 are allowed", valueP->value.s, 400);
        return false;
      }

      *mqttChangeP = true;
    }
    else
    {
      orionldError(OrionldBadRequestData, "Non-supported key in notifierInfo", keyP->value.s, 400);
      return false;
    }
  }

  return true;
}
