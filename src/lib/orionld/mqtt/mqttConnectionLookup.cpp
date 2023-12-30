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
#include <string.h>                                            // strcmp

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/MqttConnection.h"                      // MqttConnection
#include "orionld/mqtt/mqttConnectionList.h"                   // Mqtt Connection List
#include "orionld/mqtt/mqttConnectionLookup.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// mqttConnectionLookup -
//
MqttConnection* mqttConnectionLookup(const char* host, unsigned short port, const char* username, const char* password, const char* version)
{
  for (int ix = 0; ix < mqttConnectionListIx; ix++)
  {
    MqttConnection* mqP = &mqttConnectionList[ix];

    if (mqP->port != port)                                                  continue;
    if (strcmp(host, mqP->host) != 0)                                       continue;
    if ((mqP->username != NULL) && (strcmp(username, mqP->username) != 0))  continue;
    if ((mqP->password != NULL) && (strcmp(password, mqP->password) != 0))  continue;
    if ((mqP->version  != NULL) && (strcmp(version,  mqP->version)  != 0))  continue;

    if (MQTTClient_isConnected(mqP->client) != true)
      LM_T(LmtMqtt, ("Found the MQTT connection, just, it's not connected!"));
    else
      LM_T(LmtMqtt, ("Found the MQTT connection and it's connected"));

    return mqP;
  }

  return NULL;
}
