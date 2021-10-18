/*
*
* Copyright 2021 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan (with contributions from Burak Karaboga in old mqtt.cpp file)
*/

#include "mqtt/MqttConnectionManager.h"

#include <mosquitto.h>
#include <string>
#include <vector>
#include <map>

#include "logMsg/logMsg.h"
#include "common/globals.h"



/* ****************************************************************************
*
* MQTT_DEFAULT_KEEPALIVE -
*/
#define MQTT_DEFAULT_KEEPALIVE 60



/* ****************************************************************************
*
* getEndpoint -
*/
inline std::string getEndpoint(const std::string& host, int port)
{
  char  portV[STRING_SIZE_FOR_INT];
  snprintf(portV, sizeof(portV), "%d", port);
  return host + ":" + portV;
}



/* ****************************************************************************
*
* mqttOnPublishCallback -
*
*/
void mqttOnPublishCallback(struct mosquitto *mosq, void *userdata, int mid)
{
  /* mid could be used to correlate. By the moment we only print it in log traces at DEBUG log level */
  LM_T(LmtMqttNotif, ("MQTT notification successfully published at %s with id %d", (char*)userdata, mid));
}



/* ****************************************************************************
*
* MqttConnectionManager::MqttConnectionManager -
*/
MqttConnectionManager::MqttConnectionManager(void)
{
}



/* ****************************************************************************
*
* MqttConnectionManager::init -
*/
int MqttConnectionManager::init(void)
{
  LM_T(LmtMqttNotif, ("Initializing MQTT library"));
  mosquitto_lib_init();

  return semInit();
}



/* ****************************************************************************
*
* MqttConnectionManager::release -
*/
void MqttConnectionManager::release(void)
{
  LM_T(LmtMqttNotif, ("Cleanup MQTT conections"));

  // Using a negative value ensures that all connections are cleaned out
  cleanup(-1, true);

  mosquitto_lib_cleanup();
}



/* ****************************************************************************
*
* MqttConnectionManager::semInit -
*/
int MqttConnectionManager::semInit(void)
{
  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_E(("Runtime Error (error initializing 'mqtt connection mgr' semaphore: %s)", strerror(errno)));
    return -1;
  }

  return 0;
}



/* ****************************************************************************
*
* MqttConnectionManager::semGet -
*/
const char* MqttConnectionManager::semGet(void)
{
  int value;

  if (sem_getvalue(&sem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }

  return "free";
}



/* ****************************************************************************
*
* MqttConnectionManager::semTake -
*/
void MqttConnectionManager::semTake(void)
{
  sem_wait(&sem);
}



/* ****************************************************************************
*
* MqttConnectionManager::semGive -
*/
void MqttConnectionManager::semGive(void)
{
  sem_post(&sem);
}



/* ****************************************************************************
*
* MqttConnectionManager::getConnection -
*/
MqttConnection MqttConnectionManager::getConnection(const std::string& host, int port, const std::string& user, const std::string& passwd)
{
  // Note we don't take the sem here, as the caller has already done that
  std::string endpoint = getEndpoint(host, port);

  if (connections.find(endpoint) == connections.end())
  {
    // Doesn't exists: create it
     LM_T(LmtMqttNotif, ("New MQTT broker connection for %s", endpoint.c_str()));

     MqttConnection c;

     // We use userdata to store the endpoint associated to the connection, to be used later at mqttOnPublishCallback()
     // 100 should suffice for any well educated endpoint string...
     c.userdata = (char*) malloc(100);
     strncpy(c.userdata, endpoint.c_str(), 100);

     c.mosq = mosquitto_new(NULL, true, c.userdata);
     if (c.mosq == NULL)
     {
       LM_E(("Runtime Error (could not create mosquitto client instance for %s)", endpoint.c_str()));
       return c;
     }

     // Use auth is user and password has been specified
     if ((!user.empty()) && (!passwd.empty()))
     {
       int resultCode = mosquitto_username_pw_set(c.mosq, user.c_str(), passwd.c_str());
       if (resultCode != MOSQ_ERR_SUCCESS)
       {
         LM_E(("Runtime Error (could not set user/pass in MQTT Broker connection %s (%d): %s)", endpoint.c_str(), resultCode, mosquitto_strerror(resultCode)));
         mosquitto_destroy(c.mosq);
         c.mosq = NULL;
         return c;
       }
     }

     mosquitto_publish_callback_set(c.mosq, mqttOnPublishCallback);

     LM_T(LmtMqttNotif, ("Connecting to MQTT Broker at %s", endpoint.c_str()));

     // FIXME P8: user/pass errors passed unnoticed at connection timee, as
     // mosquitto_connect() always returns MOSQ_ERR_SUCCESS. There is an
     // open question about it: https://stackoverflow.com/questions/69464187/how-can-i-detect-problems-with-user-password-in-the-connection-to-mqtt-broker-at

     int resultCode = mosquitto_connect(c.mosq, host.c_str(), port, MQTT_DEFAULT_KEEPALIVE);
     if (resultCode != MOSQ_ERR_SUCCESS)
     {
       LM_E(("Runtime Error (could not connect to MQTT Broker %s (%d): %s)", endpoint.c_str(), resultCode, mosquitto_strerror(resultCode)));
       mosquitto_destroy(c.mosq);
       c.mosq = NULL;
       return c;
     }

     LM_T(LmtMqttNotif, ("Successfully connected to MQTT Broker at %s", endpoint.c_str()));

     // Starts the client loop in its own thread. The client loop processes the network traffic.
     // According to documentation (https://mosquitto.org/api/files/mosquitto-h.html#mosquitto_threaded_set):
     // "When using mosquitto_loop_start, this [mosquitto_threaded_set] is set automatically."
     mosquitto_loop_start(c.mosq);

     c.lastTime = getCurrentTime();
     connections[endpoint] = c;

     return c;
  }
  else
  {
    // Already exists: update the time counter
    LM_T(LmtMqttNotif, ("Existing MQTT broker connection for %s", endpoint.c_str()));
    connections[endpoint].lastTime = getCurrentTime();

    return connections[endpoint];
  }
}



/* ****************************************************************************
*
* MqttConnectionManager::sendMqttNotification -
*/
int MqttConnectionManager::sendMqttNotification(const std::string& host, int port, const std::string& user, const std::string& passwd, const std::string& content, const std::string& topic, unsigned int qos)
{
  // A previous version of the implementation took the sem in getConnection(), but we
  // need to do it in sendMqttNotification to avoid the connection get removed by
  // the cleanup() method while is being used here (the probability is small, but
  // it could happen in theory)
  semTake();

  MqttConnection c = getConnection(host, port, user, passwd);
  mosquitto* mosq = c.mosq;

  if (mosq == NULL)
  {
    // No need of log traces here: the getConnection() method would already print them
    semGive();
    return -1;
  }

  const char* msg = content.c_str();

  int id;

  int resultCode = mosquitto_publish(mosq, &id, topic.c_str(), (int) strlen(msg), msg, qos, false);
  if (resultCode != MOSQ_ERR_SUCCESS)
  {
    LM_E(("Runtime Error (failed to send MQTT notification (%d): %s)", resultCode, mosquitto_strerror(resultCode)));
  }
  else
  {
    LM_T(LmtMqttNotif, ("MQTT notification sent to %s:%d on topic %s with qos %d with id %d", host.c_str(), port, topic.c_str(), qos, id));
  }

  semGive();
  return 0;
}



/* ****************************************************************************
*
* MqttConnectionManager::cleanup -
*/
void MqttConnectionManager::cleanup(double maxAge, bool ignoreSems)
{
  LM_T(LmtMqttNotif, ("Checking MQTT connections age"));

  if (!ignoreSems)
  {
    semTake();
  }

  std::vector<std::string> toErase;

  for (std::map<std::string, MqttConnection>::iterator iter = connections.begin(); iter != connections.end(); ++iter)
  {
    std::string endpoint = iter->first;
    MqttConnection c     = iter->second;
    double age = getCurrentTime() - c.lastTime;
    if (age > maxAge)
    {
      LM_T(LmtMqttNotif, ("MQTT connection %s too old (age: %f, maxAge: %f), removing it", endpoint.c_str(), age, maxAge));

      toErase.push_back(endpoint);

      LM_T(LmtMqttNotif, ("Disconnecting from MQTT Broker at %s", endpoint.c_str()));
      int resultCode = mosquitto_disconnect(c.mosq);
      if (resultCode != MOSQ_ERR_SUCCESS)
      {
        LM_E(("Runtime Error (could not disconnect from MQTT Broker (%d): %s)", resultCode, mosquitto_strerror(resultCode)));
      }
      else
      {
        LM_T(LmtMqttNotif, ("Successfully disconnected from MQTT Broker at %s", endpoint.c_str()));
      }
      mosquitto_loop_stop(c.mosq, false);
      mosquitto_destroy(c.mosq);

      free(c.userdata);
    }
  }

  for (unsigned int ix = 0; ix < toErase.size(); ix++)
  {
    connections.erase(toErase[ix]);
  }

  if (!ignoreSems)
  {
    semGive();
  }
}
