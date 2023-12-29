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
#include <stdio.h>                                             // snprintf
#include <string.h>                                            // strlen, strcpy, strchr
#include <sys/uio.h>                                           // struct iovec
#include <MQTTClient.h>                                        // MQTT Client header

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjRenderSize.h"                                // kjFastRenderSize
#include "kjson/kjRender.h"                                    // kjFastRender
#include "kjson/kjBuilder.h"                                   // kjObject, kjArray, kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"

#include "cache/subCache.h"                                    // CachedSubscription

#include "orionld/common/orionldState.h"                       // orionldState, coreContextUrl
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/types/MqttConnection.h"                      // MqttConnection
#include "orionld/notifications/notificationSuccess.h"         // notificationSuccess
#include "orionld/notifications/notificationFailure.h"         // notificationFailure
#include "orionld/mqtt/mqttConnectionLookup.h"                 // mqttConnectionLookup
#include "orionld/mqtt/mqttConnectionAdd.h"                    // mqttConnectionAdd
#include "orionld/mqtt/mqttNotify.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// headersParse -
//
static KjNode* headersParse(struct iovec* ioVec, int ioVecSize, CachedSubscription* cSubP)
{
  KjNode* metadata = kjObject(orionldState.kjsonP, NULL);
  int     ix = 0;

  //
  // We skip the first line - not a header (HTTP.1/1 POST /ngsi-ld/)
  //   POST /entities?subscriptionId=urn:ngsi-ld:Subscription:mqttNotification HTTP/1.1
  // I could extract the subscription id from it and add that as a metadata ...
  // OR, I get the subscription id as a parameter
  //
  for (ix = 1; ix < ioVecSize - 2; ix++)
  {
    char* headerReadOnly = (char*) ioVec[ix].iov_base;
    int   headerLen      = ioVec[ix].iov_len;  // Includes the ending "\r\n" - which must be stripped off
    char  header[256];  // Some of the headers are hardcoded in read-only RAM, like "Content-Type: application/json" - must copy the string!

    // Some headers can be skipped
    // Some need to be encoded ...
    // * Link: lots of chars in the Link header - I could extract what's inside <> ...
    //
    if (strncmp(headerReadOnly, "Content-Length:", 15) == 0)      continue;

    if (strncmp(headerReadOnly, "Link:", 5) == 0)
    {
      const char*  link      = (cSubP->ldContext == "")? coreContextUrl : cSubP->ldContext.c_str();
      KjNode*      linkNodeP = kjString(orionldState.kjsonP, "Link", link);

      kjChildAdd(metadata, linkNodeP);
      continue;
    }

    strncpy(header, headerReadOnly, sizeof(header) - 1);

    //
    // There's a limiting line between headers and payload body. It's "\r\n"
    // As the loop doesn't include the last two (this limiting line + payload body) we should never get here.
    //
    if (header[0] == '\r')
      break;

    // The end of the line is \r\n - let's remove that
    header[headerLen - 2] = 0;

    char* colonP = strchr(header, ':');
    if (colonP == NULL)
    {
      orionldError(OrionldInternalError, "Internal Error", "no separator (:) found in HTTP header", 500);
      return NULL;
    }

    *colonP = 0;
    char* key   = header;
    char* value = &colonP[1];

    // Might be trailing spaces in the value (key: value)
    while (*value == ' ')
      ++value;

    KjNode* headerNodeP = kjString(orionldState.kjsonP, key, value);
    kjChildAdd(metadata, headerNodeP);
  }

  return metadata;
}



// -----------------------------------------------------------------------------
//
// mqttNotify -
//
int mqttNotify(CachedSubscription* cSubP, struct iovec* ioVec, int ioVecSize, double notificationTime)
{
  //
  // The headers and the body comes already rendered inside ioVec
  // That is perfect for the payload body, that would need to be rendered otherwise.
  // But, the headers need to be transformed into a KjNode tree and then rendered as JSON
  // This is easy, as every header comes in a separate slot in the iovec array, in the form;
  //   Key: Value
  // We just need to find the separator (:), null it out and create a KjString:
  //
  KjNode*  metadata = headersParse(ioVec, ioVecSize, cSubP);
  if (metadata == NULL)
  {
    orionldError(OrionldInternalError, "Internal Error", "headersParse failed", 500);
    notificationFailure(cSubP, "Error parsing headers", notificationTime);
    return -1;
  }

  int      headersLen = kjFastRenderSize(metadata);                      // Approximate
  int      totalLen   = headersLen + ioVec[ioVecSize - 1].iov_len + 10;  // Approximate
  char*    buf        = (char*) kaAlloc(&orionldState.kalloc, totalLen);

  if (buf == NULL)
  {
    orionldError(OrionldInternalError, "kaAlloc failed", "out of memory?", 500);
    notificationFailure(cSubP, "Out of memory", notificationTime);
    return -1;
  }

  strcpy(buf, "{\"metadata\":");     // strlen: 12
  kjFastRender(metadata, &buf[12]);  // adding the headers (called metadata in MQTT)
  int dataStart = strlen(buf);

  char* body = (char*) ioVec[ioVecSize - 1].iov_base;

  int bodyLen  = snprintf(&buf[dataStart], totalLen - dataStart, ",\"body\":%s}", (char*) body);
  totalLen = dataStart + bodyLen;

  MqttInfo*                 mqttP             = &cSubP->httpInfo.mqtt;
  MqttConnection*           mqttConnectionP   = mqttConnectionLookup(mqttP->host, mqttP->port, mqttP->username, mqttP->password, mqttP->version);
  MQTTClient_message        mqttMsg           = MQTTClient_message_initializer;
  MQTTClient_deliveryToken  mqttToken;

  if (mqttConnectionP == NULL)
  {
    mqttConnectionP = mqttConnectionAdd(false, mqttP->username, mqttP->password, mqttP->host, mqttP->port, mqttP->version);
    if (mqttConnectionP == NULL)
    {
      orionldError(OrionldInternalError, "MQTT Broker Problem", "unable to connect to the MQTT broker", 500);
      notificationFailure(cSubP, "Unable to connect to the MQTT broker", notificationTime);
      return -1;
    }
  }

  mqttMsg.payload    = (void*) buf;
  mqttMsg.payloadlen = totalLen;
  mqttMsg.qos        = mqttP->qos;
  mqttMsg.retained   = 0;

  LM_T(LmtMqtt, ("Sending a notification over MQTT (topic: '%s')", mqttP->topic));
  int  mr = MQTTClient_publishMessage(mqttConnectionP->client, mqttP->topic, &mqttMsg, &mqttToken);
  if (mr != MQTTCLIENT_SUCCESS)
  {
    LM_E(("MQTT Broker error %d", mr));
    // Reconnect and try again
    orionldError(OrionldInternalError, "MQTT Broker Problem", "MQTTClient_publishMessage failed", 500);
    notificationFailure(cSubP, "MQTT Broker error", notificationTime);
    return -1;
  }

  extern int  mqttTimeout;  // From mqttNotification.cpp - should be a CLI
  int rc = MQTTClient_waitForCompletion(mqttConnectionP->client, mqttToken, mqttTimeout);
  if (rc != 0)
  {
    LM_E(("Internal Error (MQTT waitForCompletion error %d)", rc));
    orionldError(OrionldInternalError, "MQTT Broker Problem", "MQTT waitForCompletion error", 500);
    notificationFailure(cSubP, "MQTT waitForCompletion error", notificationTime);
    return -1;
  }

  notificationSuccess(cSubP, notificationTime);

  return 0;
}
