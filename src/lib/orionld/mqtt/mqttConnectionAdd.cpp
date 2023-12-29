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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/types/MqttConnection.h"                      // MqttConnection
#include "orionld/mqtt/mqttConnect.h"                          // mqttConnect
#include "orionld/mqtt/mqttConnectionList.h"                   // Mqtt Connection List
#include "orionld/mqtt/mqttConnectionAdd.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// mqttConnectionAdd -
//
MqttConnection* mqttConnectionAdd
(
  bool           mqtts,
  const char*    username,
  const char*    password,
  const char*    host,
  unsigned short port,
  const char*    version
)
{
  MqttConnection* mqP = NULL;

  // Any empty slots?
  for (int ix = 0; ix < mqttConnectionListIx; ix++)
  {
    if (mqttConnectionList[ix].host == NULL)
    {
      mqP = &mqttConnectionList[ix];
      break;
    }
  }

  if (mqP == NULL)
  {
    if (mqttConnectionListIx >= mqttConnectionListSize)
    {
      mqttConnectionListSize += 20;
      mqttConnectionList      = (MqttConnection*) realloc(mqttConnectionList, sizeof(MqttConnection) * mqttConnectionListSize);
    }

    mqP = &mqttConnectionList[mqttConnectionListIx];
  }

  if (mqP == NULL)
  {
    LM_E(("Internal Error (no MQTT connection available)"));
    return NULL;
  }

  mqP->host     = strdup(host);
  mqP->port     = port;
  mqP->username = (username != NULL)? strdup(username) : NULL;
  mqP->password = (password != NULL)? strdup(password) : NULL;
  mqP->version  = (version  != NULL)? strdup(version)  : NULL;

  if (mqttConnect(mqP, mqtts, username, password, host, port, version) == false)
  {
    LM_E(("Internal Error (mqttConnect failed)"));
    return NULL;
  }

  ++mqttConnectionListIx;

  return mqP;
}
