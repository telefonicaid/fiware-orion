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
#include "MQTTClient.h"                                        // MQTT Client header

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/mqtt/MqttConnection.h"                       // MqttConnection
#include "orionld/mqtt/mqttConnectionList.h"                   // Mqtt Connection List
#include "orionld/mqtt/mqttConnect.h"                          // Own interface



// -----------------------------------------------------------------------------
//
// mqttConnect -
//
bool mqttConnect(MqttConnection* mqP, bool mqtts, const char* username, const char* password, const char* host, unsigned short port)
{
  MQTTClient_connectOptions  connectOptions = MQTTClient_connectOptions_initializer;
  char                       address[64];
  int                        status;

  snprintf(address, sizeof(address), "%s:%d", host, port);
  MQTTClient_create(&mqP->client, address, "Orion-LD", MQTTCLIENT_PERSISTENCE_NONE, NULL);
  connectOptions.keepAliveInterval = 20;
  connectOptions.cleansession      = 1;
  connectOptions.username          = username;
  connectOptions.password          = password;

  if (mqtts)
  {
    LM_W(("WARNING - MQTT/SSL is not implemented yet - using unsecure MQTT for now. Sorry ... "));
  }

  LM_TMP(("MQTT: Connecting to MQTT server on '%s'", address));
  if ((status = MQTTClient_connect(mqP->client, &connectOptions)) != MQTTCLIENT_SUCCESS)
  {
    LM_E(("Internal Error (unable to connect to MQTT server (%s:%d): MQTTClient_connect error %d", host, port, status));
    return false;
  }

  LM_TMP(("MQTT: Connected to the MQTT Server at %s:%d", host, port));
  return true;
}
