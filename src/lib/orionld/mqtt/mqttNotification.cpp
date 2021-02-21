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
#include <string.h>                                            // strlen
#include <MQTTClient.h>                                        // MQTT Client header
#include <string>                                              // std::string
#include <map>                                                 // std::map

extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/kjRender.h"                                    // kjFastRender
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/mqtt/MqttConnection.h"                       // MqttConnection
#include "orionld/mqtt/mqttConnectionLookup.h"                 // mqttConnectionLookup
#include "orionld/mqtt/mqttNotification.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// mqttTimeout
//
int  mqttTimeout = 10000;  // FIXME: This variable should be a CLI for Orion-LD



// -----------------------------------------------------------------------------
//
// mqttNotification -
//
int mqttNotification
(
  const char*                         host,
  unsigned short                      port,
  const char*                         topic,
  const char*                         body,
  const char*                         mimeType,
  int                                 QoS,
  const char*                         username,
  const char*                         password,
  const char*                         mqttVersion,
  const char*                         xauthToken,
  std::map<std::string, std::string>& extraHeaders
)
{
  KjNode*  metadataNodeP      = kjObject(orionldState.kjsonP, "metadata");
  KjNode*  contentTypeNodeP   = kjString(orionldState.kjsonP, "Content-Type", mimeType);
  char*    metadataBuf        = kaAlloc(&orionldState.kalloc, 4096);
  int      totalLen;
  char*    totalBuf;

  if ((metadataNodeP == NULL) || (contentTypeNodeP == NULL) || (metadataBuf == NULL))
  {
    LM_E(("Internal Error (unable to allocate)"));
    return -1;
  }

  kjChildAdd(metadataNodeP, contentTypeNodeP);

  bool     xauthTokenIncluded = false;
  if ((xauthToken != NULL) && (*xauthToken != 0))
  {
    KjNode* xauthTokenNodeP = kjString(orionldState.kjsonP, "X-Auth-Token", xauthToken);
    kjChildAdd(metadataNodeP, xauthTokenNodeP);
    xauthTokenIncluded = true;
  }


  //
  // Extra headers
  //
  for (std::map<std::string, std::string>::const_iterator it = extraHeaders.begin(); it != extraHeaders.end(); ++it)
  {
    char*  key   = (char*) it->first.c_str();
    char*  value = (char*) it->second.c_str();

    if (strcmp(key, "Link") == 0)
    {
      //
      // Link: <PATH>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"
      //
      // We only want the "LINK" value - the rest fucks up the JSON
      //
      char* closingBracket = strchr(value, '>');
      char* openingBracket = strchr(value, '<');

      if ((openingBracket == NULL) || (closingBracket == NULL))
      {
        LM_E(("Internal Error (invalid Link header: '%s')", value));
        continue;
      }

      value = &openingBracket[1];
      *closingBracket = 0;
    }
    else if ((xauthTokenIncluded == true) && (strcmp(key, "X-Auth-Token") == 0))
      continue;

    KjNode* kvP = kjString(orionldState.kjsonP, key, value);
    kjChildAdd(metadataNodeP, kvP);
  }


  kjFastRender(orionldState.kjsonP, metadataNodeP, metadataBuf, 4096);

  totalLen = strlen(body) + strlen(metadataBuf) + 50;
  totalBuf = kaAlloc(&orionldState.kalloc, totalLen);

  snprintf(totalBuf, totalLen, "{\"metadata\": %s,\"body\": %s}", metadataBuf, body);

  MqttConnection*           mqttP   = mqttConnectionLookup(host, port, username, password, mqttVersion);
  MQTTClient_message        mqttMsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken  mqttToken;

  if (mqttP == NULL)
  {
    LM_E(("Internal Error (MQTT connection for %s:%d not found)", host, port));
    return -1;
  }

  mqttMsg.payload    = (void*) totalBuf;
  mqttMsg.payloadlen = strlen(totalBuf);
  mqttMsg.qos        = QoS;
  mqttMsg.retained   = 0;

  MQTTClient_publishMessage(mqttP->client, topic, &mqttMsg, &mqttToken);

  int rc = MQTTClient_waitForCompletion(mqttP->client, mqttToken, mqttTimeout);
  if (rc != 0)
  {
    LM_E(("Internal Error (MQTT waitForCompletion error %d)", rc));
    return -1;
  }

  return 0;
}
