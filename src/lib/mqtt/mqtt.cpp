/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Burak Karaboga
*/
#include <mosquitto.h>

#include "logMsg/logMsg.h"
#include "mqtt/mqtt.h"



/* ****************************************************************************
*
* Globals
*/
struct mosquitto *mosq;
const char*      mqttHostname;
unsigned short   mqttPortNumber;
unsigned short   mqttKeepAlivePeriod;



/* ****************************************************************************
*
* mqttOnPublishCallback -
* 
*/
void mqttOnPublishCallback(struct mosquitto *mosq, void *userdata, int mid)
{
  /* FIXME: We don't assign message ids to the published notifications and therefore we have 
   * no way to tell which notification a callback log belongs to. - Planned for second MQTT PR. */
  LM_I(("MQTT notification successfully published on %s:%d", mqttHostname, mqttPortNumber));
}



/* ****************************************************************************
*
* mqttInit -
*
*/
void mqttInit (
  const char*    _mqttHost,
  unsigned short _mqttPort,
  unsigned short _mqttKeepAlive
)
{
  mqttHostname = _mqttHost;
  mqttPortNumber = _mqttPort;
  mqttKeepAlivePeriod = _mqttKeepAlive;

  LM_I(("Initializing MQTT client."));
  
  mosquitto_lib_init();
  mosq = mosquitto_new(NULL, true, NULL);
  if (mosq == NULL)
  {
    LM_X(1, ("Fatal Error (Could not create Mosquitto client instance.)"));
  }

  mosquitto_publish_callback_set(mosq, mqttOnPublishCallback);

  LM_I(("Connecting to MQTT Broker at %s:%d", _mqttHost, _mqttPort));

  int resultCode = mosquitto_connect(mosq, _mqttHost, _mqttPort, _mqttKeepAlive);
  if (resultCode != MOSQ_ERR_SUCCESS)
  {
    LM_X(2, ("Fatal Error (Could not connect to MQTT Broker (%d): %s)", resultCode, mosquitto_strerror(resultCode)));
  }
  
  LM_I(("Successfully connected to MQTT Broker."));

  // Starts the client loop in its own thread. The client loop processes the network traffic
  mosquitto_loop_start(mosq);
}



/* ****************************************************************************
*
* sendMqttNotification -
*
*/
int sendMqttNotification(const std::string& content)
{
  const char* msg = content.c_str();

  // FIXME: unhardwire QoS, retain and topic - Planned for second MQTT PR
  int resultCode = mosquitto_publish(mosq, NULL, "orion", (int) strlen(msg), msg, 0, false);
  if (resultCode != MOSQ_ERR_SUCCESS)
  {
    LM_E(("Failed to send MQTT notification (%d): %s", resultCode, mosquitto_strerror(resultCode)));
  }
  else {
    LM_I(("MQTT notification sent to %s:%d", mqttHostname, mqttPortNumber));
  }

  return 0;
}



/* ****************************************************************************
*
* mqttCleanup -
*
*/
void mqttCleanup(void)
{
  LM_I(("Disconnecting from MQTT Broker"));
  int resultCode = mosquitto_disconnect(mosq);
  if (resultCode != MOSQ_ERR_SUCCESS) 
  {
    LM_W(("Could not disconnect from MQTT Broker (%d): %s", resultCode, mosquitto_strerror(resultCode)));
  }
  else
  {
    LM_I(("Successfully disconnected from MQTT Broker"));
  }
  mosquitto_loop_stop(mosq, false);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
}