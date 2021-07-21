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

#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/payloadCheck/pcheckUri.h"                    // pcheckUri
#include "orionld/mqtt/mqttCheck.h"                            // mqttCheck
#include "orionld/kjTree/kjTreeToEndpoint.h"                   // Own interface



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
        LM_E(("Bad Input (Invalid Endpoint::receiverInfo field: '%s')", nodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Endpoint::receiverInfo field", nodeP->name);
        return false;
      }
    }

    if ((key == NULL) || (value == NULL))
    {
        LM_E(("Bad Input (Incomplete Endpoint::receiverInfo key-value pair)"));
        orionldErrorResponseCreate(OrionldBadRequestData, "Bad Input", "Incomplete Endpoint::receiverInfo key-value pair");
        return false;
    }

    httpInfoP->headers[key] = value;
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
        LM_E(("Bad Input (Invalid Endpoint::notifierInfo field: '%s')", nodeP->name));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Endpoint::notifierInfo field", nodeP->name);
        return false;
      }
    }

    if ((key == NULL) || (value == NULL))
    {
      LM_W(("Bad Input (Incomplete Endpoint::notifierInfo key-value pair - one of them is missing)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad Input", "Incomplete Endpoint::notifierInfo key-value pair - one of them is missing");
      return false;
    }

    if ((*key == 0) || (*value == 0))
    {
      LM_W(("Bad Input (Incomplete Endpoint::notifierInfo key-value pair - one of them is empty)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad Input", "Incomplete Endpoint::notifierInfo key-value pair - one of them is empty");
      return false;
    }

    KeyValue* keyValueP = (KeyValue*) kaAlloc(&orionldState.kalloc, sizeof(KeyValue));

    strncpy(keyValueP->key,   key,   sizeof(keyValueP->key));
    strncpy(keyValueP->value, value, sizeof(keyValueP->value));
    httpInfoP->notifierInfo.push_back(keyValueP);

    //
    // Known key-values are extracted
    //
    if (strcmp(key, "MQTT-Version") == 0)
    {
      if ((strcmp(value, "mqtt3.1.1") != 0) && (strcmp(value, "mqtt5.0") != 0))
      {
        LM_W(("Bad Input (Invalid value for MQTT-Version in Endpoint::notifierInfo key-value pair - '%s')", value));
        orionldErrorResponseCreate(OrionldBadRequestData, "Bad Input", "Invalid value for MQTT-Version Endpoint::notifierInfo key-value pair");
        return false;
      }

      strncpy(httpInfoP->mqtt.version, value, sizeof(httpInfoP->mqtt.version));
    }
    else if (strcmp(key, "MQTT-QoS") == 0)
    {
      if      ((value[0] == '0') && (value[1] == 0))   httpInfoP->mqtt.qos = 0;
      else if ((value[0] == '1') && (value[1] == 0))   httpInfoP->mqtt.qos = 1;
      else if ((value[0] == '2') && (value[1] == 0))   httpInfoP->mqtt.qos = 2;
      else
      {
        LM_W(("Bad Input (Invalid value for MQTT-QoS in Endpoint::notifierInfo key-value pair - '%s')", value));
        orionldErrorResponseCreate(OrionldBadRequestData, "Bad Input", "Invalid value for MQTT-QoS Endpoint::notifierInfo key-value pair");
        return false;
      }
    }
    else
    {
      LM_W(("Bad Input (Invalid key in Endpoint::notifierInfo)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad Input", "Invalid key in Endpoint::notifierInfo");
      return false;
    }
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
  httpInfoP->mimeType = JSON;

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
          LM_W(("Bad Input (endpoint is not a valid MQTT endpoint)"));
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Endpoint::uri", "Endpoint is not a valid MQTT endpoint");
          return false;
        }
      }
      else if (pcheckUri(uriP, true, &detail) == false)
      {
        LM_W(("Bad Input (endpoint is not a valid URI)"));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Endpoint::uri", "Endpoint is not a valid URI");
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
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Endpoint::accept value", mimeType);
        return false;
      }

      // Already accepted the first 12 chars (application/), step over them
      mimeType = &mimeType[12];
      if (SCOMPARE5(mimeType, 'j', 's', 'o', 'n', 0))
        httpInfoP->mimeType = JSON;
      else if (SCOMPARE8(mimeType, 'l', 'd', '+', 'j', 's', 'o', 'n', 0))
        httpInfoP->mimeType = JSONLD;
      else if (SCOMPARE9(mimeType, 'g', 'e', 'o', '+', 'j', 's', 'o', 'n', 0))
        httpInfoP->mimeType = GEOJSON;
      else
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Endpoint::accept value", itemP->value.s);
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
      orionldErrorResponseCreate(OrionldBadRequestData, "Unrecognized field in Endpoint", itemP->name);
      return false;
    }
  }

  if (uriP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Mandatory field missing", "Subscription::notification::endpoint::uri");
    return false;
  }

  return true;
}
