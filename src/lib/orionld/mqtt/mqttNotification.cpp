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

extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/kjRender.h"                                    // kjRender
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
//
//
int  mqttQos    = 1;      // FIXME: This variable should be a CLI for Orion-LD
int  mqttTimout = 10000;  // FIXME: This variable should be a CLI for Orion-LD



// -----------------------------------------------------------------------------
//
// mqttNotification -
//
int mqttNotification(const char* host, unsigned short port, const char* topic, const char* body, const char* mimeType)
{
  KjNode*  metadataNodeP     = kjObject(orionldState.kjsonP, "metadata");
  KjNode*  contentTypeNodeP  = kjString(orionldState.kjsonP, "Content-Type", mimeType);
  char*    metadataBuf       = kaAlloc(&orionldState.kalloc, 1024);
  int      totalLen;
  char*    totalBuf;

  if ((metadataNodeP == NULL) || (contentTypeNodeP == NULL) || (metadataBuf == NULL))
  {
    LM_E(("Internal Error (unable to allocate)"));
    return -1;
  }

  LM_TMP(("MQTT: In mqttNotification"));
  kjChildAdd(metadataNodeP, contentTypeNodeP);
  LM_TMP(("MQTT: Calling kjRender"));
  kjRender(orionldState.kjsonP, metadataNodeP, metadataBuf, 1024);
  LM_TMP(("MQTT: After kjRender"));
  LM_TMP(("MQTT: metadataBuf: '%s'", metadataBuf));

  totalLen = strlen(body) + strlen(metadataBuf) + 50;
  totalBuf = kaAlloc(&orionldState.kalloc, totalLen);

  snprintf(totalBuf, totalLen, "{\"metadata\": %s,\"body\": %s}", metadataBuf, body);

  MqttConnection*           mqttP   = mqttConnectionLookup(host, port);
  MQTTClient_message        mqttMsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken  mqttToken;

  if (mqttP == NULL)
  {
    LM_E(("Internal Error (MQTT connection for %s:%d not found)", host, port));
    return -1;
  }

  LM_TMP(("MQTT: Found MQTT connection for %s:%d", mqttP->host, mqttP->port));

  mqttMsg.payload    = (void*) totalBuf;
  mqttMsg.payloadlen = strlen(totalBuf);
  mqttMsg.qos        = mqttQos;
  mqttMsg.retained   = 0;

  LM_TMP(("MQTT: Sending MQTT notification to %s:%d (on topic: %s): %s", host, port, topic, totalBuf));
  MQTTClient_publishMessage(mqttP->client, topic, &mqttMsg, &mqttToken);

  LM_TMP(("MQTT: Waiting for (up to) %d milliseconds for topic '%s' meesage", mqttTimout, topic));
  int rc = MQTTClient_waitForCompletion(mqttP->client, mqttToken, mqttTimout);
  if (rc != 0)
  {
    LM_E(("Internal Error (MQTT waitForCompletion error %d)", rc));
    return -1;
  }

  LM_TMP(("MQTT: All OK"));
  return 0;
}
