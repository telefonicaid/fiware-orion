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
#include <string>                                              // std::string
#include <vector>                                              // std::vector

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kalloc/kaAlloc.h"                                    // kaAlloc
}

#include "apiTypesV2/HttpInfo.h"                               // HttpInfo

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/payloadCheck/pCheckUri.h"                    // pCheckUri
#include "orionld/mqtt/mqttCheck.h"                            // mqttCheck
#include "orionld/legacyDriver/kjTreeToEndpoint.h"             // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeToReceiverInfo -
//
static bool kjTreeToReceiverInfo(KjNode* receiverInfoP, ngsiv2::HttpInfo* httpInfoP)
{
  for (KjNode* kvP = receiverInfoP->value.firstChildP; kvP != NULL; kvP = kvP->next)
  {
    char* key   = NULL;
    char* value = NULL;

    OBJECT_CHECK(kvP, "receiverInfo key-value");

    for (KjNode* nodeP = kvP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (SCOMPARE4(nodeP->name, 'k', 'e', 'y', 0))
      {
        DUPLICATE_CHECK(key, "Endpoint::receiverInfo::key", nodeP->value.s);
        STRING_CHECK(nodeP, "Endpoint::receiverInfo::key");
      }
      else if (SCOMPARE6(nodeP->name, 'v', 'a', 'l', 'u', 'e', 0))
      {
        DUPLICATE_CHECK(value, "Endpoint::receiverInfo::value", nodeP->value.s);
        STRING_CHECK(nodeP, "Endpoint::receiverInfo::value");
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid Endpoint::receiverInfo field", nodeP->name, 400);
        return false;
      }
    }

    if ((key == NULL) || (value == NULL))
    {
        orionldError(OrionldBadRequestData, "Bad Input", "Incomplete Endpoint::receiverInfo key-value pair", 400);
        return false;
    }

    if ((*key == 0) || (*value == 0))
    {
      orionldError(OrionldBadRequestData, "Bad Input", "Incomplete Endpoint::receiverInfo key-value pair - one of them is empty", 400);
      return false;
    }

    httpInfoP->headers[key] = value;  // Adding the key-value to the list of headers for outgoing notifications
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToNotifierInfo -
//
static bool kjTreeToNotifierInfo(KjNode* notifierInfoP, ngsiv2::HttpInfo* httpInfoP)
{
  for (KjNode* kvP = notifierInfoP->value.firstChildP; kvP != NULL; kvP = kvP->next)
  {
    char* key   = NULL;
    char* value = NULL;

    OBJECT_CHECK(kvP, "notifierInfo key-value");

    for (KjNode* nodeP = kvP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (SCOMPARE4(nodeP->name, 'k', 'e', 'y', 0))
      {
        DUPLICATE_CHECK(key, "Endpoint::notifierInfo::key", nodeP->value.s);
        STRING_CHECK(nodeP, "Endpoint::notifierInfo::key");
      }
      else if (SCOMPARE6(nodeP->name, 'v', 'a', 'l', 'u', 'e', 0))
      {
        DUPLICATE_CHECK(value, "Endpoint::notifierInfo::value", nodeP->value.s);
        STRING_CHECK(nodeP, "Endpoint::notifierInfo::value");
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid Endpoint::notifierInfo field", nodeP->name, 400);
        return false;
      }
    }

    if ((key == NULL) || (value == NULL))
    {
      orionldError(OrionldBadRequestData, "Bad Input", "Incomplete Endpoint::notifierInfo key-value pair - one of them is missing", 400);
      return false;
    }

    if ((*key == 0) || (*value == 0))
    {
      orionldError(OrionldBadRequestData, "Bad Input", "Incomplete Endpoint::notifierInfo key-value pair - one of them is empty", 400);
      return false;
    }

    //
    // Known key-values are extracted
    //
    if (strcmp(key, "MQTT-Version") == 0)
    {
      if ((strcmp(value, "mqtt3.1.1") != 0) && (strcmp(value, "mqtt5.0") != 0))
      {
        orionldError(OrionldBadRequestData, "Bad Input", "Invalid value for MQTT-Version Endpoint::notifierInfo key-value pair", 400);
        return false;
      }

      strncpy(httpInfoP->mqtt.version, value, sizeof(httpInfoP->mqtt.version) - 1);
    }
    else if (strcmp(key, "MQTT-QoS") == 0)
    {
      if      ((value[0] == '0') && (value[1] == 0))   httpInfoP->mqtt.qos = 0;
      else if ((value[0] == '1') && (value[1] == 0))   httpInfoP->mqtt.qos = 1;
      else if ((value[0] == '2') && (value[1] == 0))   httpInfoP->mqtt.qos = 2;
      else
      {
        orionldError(OrionldBadRequestData, "Bad Input", "Invalid value for MQTT-QoS Endpoint::notifierInfo key-value pair", 400);
        return false;
      }
    }
    else
    {
      orionldError(OrionldBadRequestData, "Bad Input", "Invalid key in Endpoint::notifierInfo", 400);
      return false;
    }

    //
    // Seems a bit stupid to allocate room for copies here ...
    // However, the current implementation needs this to work.
    // Should be easy enough to fix (and avoid unnecessary allocations)
    //
    keyValueAdd(&httpInfoP->notifierInfo, key, value);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToEndpoint -
//
bool kjTreeToEndpoint(KjNode* kNodeP, ngsiv2::HttpInfo* httpInfoP)
{
  char*   uriP          = NULL;
  char*   acceptP       = NULL;
  KjNode* receiverInfoP = NULL;
  KjNode* notifierInfoP = NULL;
  char*   detail;

  // Set default values
  httpInfoP->mimeType = MT_JSON;

  for (KjNode* itemP = kNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE4(itemP->name, 'u', 'r', 'i', 0))
    {
      DUPLICATE_CHECK(uriP, "Endpoint::uri", itemP->value.s);
      STRING_CHECK(itemP, "Endpoint::uri");

      if (strncmp(uriP, "mqtt://", 7) == 0)
      {
        if (mqttCheck(uriP, &detail) == false)
        {
          orionldError(OrionldBadRequestData, "Invalid Endpoint::uri", "Endpoint is not a valid MQTT endpoint", 400);
          return false;
        }
      }
      else if (pCheckUri(uriP, "Endpoint::uri", true) == false)
      {
        orionldError(OrionldBadRequestData, "Invalid Endpoint::uri", "Endpoint is not a valid URI", 400);
        return false;
      }

      httpInfoP->url = uriP;
    }
    else if (SCOMPARE7(itemP->name, 'a', 'c', 'c', 'e', 'p', 't', 0))
    {
      DUPLICATE_CHECK(acceptP, "Endpoint::accept", itemP->value.s);
      STRING_CHECK(itemP, "Endpoint::accept");
      char* mimeType = itemP->value.s;

      if (!SCOMPARE12(mimeType, 'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/'))
      {
        orionldError(OrionldBadRequestData, "Invalid Endpoint::accept value", mimeType, 400);
        return false;
      }

      // Already accepted the first 12 chars (application/), step over them
      mimeType = &mimeType[12];
      if (SCOMPARE5(mimeType, 'j', 's', 'o', 'n', 0))
        httpInfoP->mimeType = MT_JSON;
      else if (SCOMPARE8(mimeType, 'l', 'd', '+', 'j', 's', 'o', 'n', 0))
        httpInfoP->mimeType = MT_JSONLD;
      else if (SCOMPARE9(mimeType, 'g', 'e', 'o', '+', 'j', 's', 'o', 'n', 0))
        httpInfoP->mimeType = MT_GEOJSON;
      else
      {
        orionldError(OrionldBadRequestData, "Invalid Endpoint::accept value", itemP->value.s, 400);
        return false;
      }
    }
    else if (SCOMPARE13(itemP->name, 'r', 'e', 'c', 'e', 'i', 'v', 'e', 'r', 'I', 'n', 'f', 'o', 0))
    {
      DUPLICATE_CHECK(receiverInfoP, "Endpoint::receiverInfo", itemP);
      ARRAY_CHECK(itemP, "Endpoint::receiverInfo");

      if (kjTreeToReceiverInfo(receiverInfoP, httpInfoP) == false)
        return false;
    }
    else if (SCOMPARE13(itemP->name, 'n', 'o', 't', 'i', 'f', 'i', 'e', 'r', 'I', 'n', 'f', 'o', 0))
    {
      DUPLICATE_CHECK(notifierInfoP, "Endpoint::notifierInfo", itemP);
      ARRAY_CHECK(itemP, "Endpoint::notifierInfo");

      if (kjTreeToNotifierInfo(notifierInfoP, httpInfoP) == false)
        return false;
    }
    else
    {
      orionldError(OrionldBadRequestData, "Unrecognized field in Endpoint", itemP->name, 400);
      return false;
    }
  }

  if (uriP == NULL)
  {
    orionldError(OrionldBadRequestData, "Mandatory field missing", "Subscription::notification::endpoint::uri", 400);
    return false;
  }

  return true;
}
