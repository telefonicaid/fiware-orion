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

#include <string>

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


#if 0
/* ****************************************************************************
*
* mqttOnPublishCallback -
* 
*/
void mqttOnPublishCallback(struct mosquitto *mosq, void *userdata, int mid)
{
  /* mid could be used to correlate. By the moment we only print it in log traces at DEBUG log level */
  LM_T(LmtMqttNotif, ("MQTT notification successfully published on %s:%d with id %d", mqttHostname, mqttPortNumber, mid));
}
#endif



/* ****************************************************************************
*
* mqttInit -
*
*/
void mqttInit
(
  const char*    _mqttHost,
  unsigned short _mqttPort,
  unsigned short _mqttKeepAlive
)
{
  mqttHostname = _mqttHost;
  mqttPortNumber = _mqttPort;
  mqttKeepAlivePeriod = _mqttKeepAlive;

  LM_T(LmtMqttNotif, ("Initializing MQTT client"));

  mosquitto_lib_init();
  mosq = mosquitto_new(NULL, true, NULL);
  if (mosq == NULL)
  {
    LM_X(1, ("Fatal Error (Could not create Mosquitto client instance.)"));
  }

  //mosquitto_publish_callback_set(mosq, mqttOnPublishCallback);

  LM_T(LmtMqttNotif, ("Connecting to MQTT Broker at %s:%d", _mqttHost, _mqttPort));

  int resultCode = mosquitto_connect(mosq, _mqttHost, _mqttPort, _mqttKeepAlive);
  if (resultCode != MOSQ_ERR_SUCCESS)
  {
    LM_X(2, ("Fatal Error (Could not connect to MQTT Broker (%d): %s)", resultCode, mosquitto_strerror(resultCode)));
  }

  LM_T(LmtMqttNotif, ("Successfully connected to MQTT Broker"));

  // Starts the client loop in its own thread. The client loop processes the network traffic.
  // According to documentation (https://mosquitto.org/api/files/mosquitto-h.html#mosquitto_threaded_set):
  // "When using mosquitto_loop_start, this [mosquitto_threaded_set] is set automatically."
  mosquitto_loop_start(mosq);
}



/* ****************************************************************************
*
* sendMqttNotification -
*
*/
int sendMqttNotification(const std::string& content, const std::string& topic, unsigned int qos)
{
  const char* msg = content.c_str();

  int id;

  int resultCode = mosquitto_publish(mosq, &id, topic.c_str(), (int) strlen(msg), msg, qos, false);
  if (resultCode != MOSQ_ERR_SUCCESS)
  {
    LM_E(("Failed to send MQTT notification (%d): %s", resultCode, mosquitto_strerror(resultCode)));
  }
  else
  {
    LM_T(LmtMqttNotif, ("MQTT notification sent to %s:%d on topic %s with qos %d with id %d", mqttHostname, mqttPortNumber, topic.c_str(), qos, id));
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
  LM_T(LmtMqttNotif, ("Disconnecting from MQTT Broker"));
  int resultCode = mosquitto_disconnect(mosq);
  if (resultCode != MOSQ_ERR_SUCCESS)
  {
    LM_E(("Could not disconnect from MQTT Broker (%d): %s", resultCode, mosquitto_strerror(resultCode)));
  }
  else
  {
    LM_T(LmtMqttNotif, ("Successfully disconnected from MQTT Broker"));
  }
  mosquitto_loop_stop(mosq, false);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
}
