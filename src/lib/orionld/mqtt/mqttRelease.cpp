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
#include <stdlib.h>                                            // free

#include "orionld/mqtt/MqttConnection.h"                       // MqttConnection
#include "orionld/mqtt/mqttConnectionList.h"                   // mqttConnectionList
#include "orionld/mqtt/mqttRelease.h"                          // Own Interface



// -----------------------------------------------------------------------------
//
// mqttRelease -
//
void mqttRelease(void)
{
  if (mqttConnectionList == NULL)
    return;

  for (int ix = 0; ix < mqttConnectionListIx; ix++)
  {
    MqttConnection* mcP = &mqttConnectionList[ix];

    if (mcP->host     != NULL)      free(mcP->host);
    if (mcP->username != NULL)      free(mcP->username);
    if (mcP->password != NULL)      free(mcP->password);
    if (mcP->version  != NULL)      free(mcP->version);

    MQTTClient_disconnect(&mcP->client, 10000);
    MQTTClient_destroy(&mcP->client);
  }

  free(mqttConnectionList);
  mqttConnectionList = NULL;
}
