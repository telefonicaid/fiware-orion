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
#include "alarmMgr/alarmMgr.h"



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
* mqttOnConnectCallback -
*/
void mqttOnConnectCallback(struct mosquitto* mosq, void *userdata, int rc)
{
  MqttConnection* cP = (MqttConnection*) userdata;

  // To be used in getConnetion()
  cP->conectionResult = rc;

  // Signal that callback has been called
  cP->connectionCallbackCalled = true;

  // This allows the code waiting in getConnection() to continue
  sem_post(&cP->connectionSem);
}



/* ****************************************************************************
*
* mqttOnPublishCallback -*/
void mqttOnPublishCallback(struct mosquitto *mosq, void *userdata, int mid)
{
  MqttConnection* cP = (MqttConnection*) userdata;

  // mid could be used to correlate. By the moment we only print it in log traces at DEBUG log level
  // Note this trace use N/A in corrid= and transid=
  LM_T(LmtMqttNotif, ("MQTT notification successfully published at %s with id %d", cP->endpoint.c_str(), mid));
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
void MqttConnectionManager::init(long _timeout)
{
  LM_T(LmtMqttNotif, ("Initializing MQTT library"));
  mosquitto_lib_init();

  if (_timeout != -1)
  {
    timeout = _timeout;
  }
  else
  {
    timeout = DEFAULT_TIMEOUT;
  }

  semInit();
}



/* ****************************************************************************
*
* MqttConnectionManager::teardown -
*/
void MqttConnectionManager::teardown(void)
{
  LM_T(LmtMqttNotif, ("Teardown MQTT connections"));

  for (std::map<std::string, MqttConnection*>::iterator iter = connections.begin(); iter != connections.end(); ++iter)
  {
    std::string endpoint = iter->first;
    MqttConnection* cP   = iter->second;

    disconnect(cP->mosq, endpoint);
    cP->mosq = NULL;

    delete cP;
  }

  mosquitto_lib_cleanup();
}



/* ****************************************************************************
*
* MqttConnectionManager::semInit -
*/
void MqttConnectionManager::semInit(void)
{
  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_X(1, ("Fatal Error (error initializing 'mqtt connection mgr' semaphore: %s)", strerror(errno)));
  }
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
* MqttConnectionManager::disconnect -
*/
void MqttConnectionManager::disconnect(struct mosquitto*  mosq, const std::string& endpoint)
{
  LM_T(LmtMqttNotif, ("Disconnecting from MQTT Broker at %s", endpoint.c_str()));
  int resultCode = mosquitto_disconnect(mosq);

  // Sometimes this function is called when connection is not actually stablisehd, so we skip logging
  // the MOSQ_ERR_NO_CONN case
  if ((resultCode != MOSQ_ERR_SUCCESS) && (resultCode != MOSQ_ERR_NO_CONN))
  {
    LM_E(("Runtime Error (could not disconnect from MQTT Broker (%d): %s)", resultCode, mosquitto_strerror(resultCode)));
  }
  else
  {
    LM_T(LmtMqttNotif, ("Successfully disconnected from MQTT Broker at %s", endpoint.c_str()));
  }
  mosquitto_loop_stop(mosq, false);
  mosquitto_destroy(mosq);
}



/* ****************************************************************************
*
* MqttConnectionManager::getConnection -
*/
MqttConnection* MqttConnectionManager::getConnection(const std::string& host, int port, const std::string& user, const std::string& passwd)
{
  // Note we don't take the sem here, as the caller has already done that
  std::string endpoint = getEndpoint(host, port);

  if (connections.find(endpoint) == connections.end())
  {
    // Doesn't exists: create it
     LM_T(LmtMqttNotif, ("New MQTT broker connection for %s", endpoint.c_str()));

     MqttConnection* cP = new MqttConnection();

     cP->endpoint = endpoint;

     // Note sem is started at 0, so next sem_wait will block
     if (sem_init(&cP->connectionSem, 0, 0) == -1)
     {
       LM_E(("Runtime Error (error initializing connection sem for %s: %s)", endpoint.c_str(), strerror(errno)));
       cP->mosq = NULL;
       return cP;
     }

     cP->mosq = mosquitto_new(NULL, true, cP);
     if (cP->mosq == NULL)
     {
       LM_E(("Runtime Error (could not create mosquitto client instance for %s)", endpoint.c_str()));
       return cP;
     }

     // Use auth is user and password has been specified
     if ((!user.empty()) && (!passwd.empty()))
     {
       int resultCode = mosquitto_username_pw_set(cP->mosq, user.c_str(), passwd.c_str());
       if (resultCode != MOSQ_ERR_SUCCESS)
       {
         LM_E(("Runtime Error (could not set user/pass in MQTT Broker connection %s (%d): %s)", endpoint.c_str(), resultCode, mosquitto_strerror(resultCode)));
         mosquitto_destroy(cP->mosq);
         cP->mosq = NULL;
         return cP;
       }
     }

     // We need this for connection timeout
     struct timespec  ts;
     if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
     {
       LM_E(("Runtime Error (clock_gettime: %s)", strerror(errno)));
       mosquitto_destroy(cP->mosq);
       cP->mosq = NULL;
       return cP;
     }

     mosquitto_connect_callback_set(cP->mosq, mqttOnConnectCallback);
     mosquitto_publish_callback_set(cP->mosq, mqttOnPublishCallback);

     LM_T(LmtMqttNotif, ("Connecting to MQTT Broker at %s", endpoint.c_str()));

     int resultCode = mosquitto_connect(cP->mosq, host.c_str(), port, MQTT_DEFAULT_KEEPALIVE);
     if (resultCode != MOSQ_ERR_SUCCESS)
     {
       // Alarm is raised in this case
       alarmMgr.mqttConnectionError(endpoint, mosquitto_strerror(resultCode));
       mosquitto_destroy(cP->mosq);
       cP->mosq = NULL;
       return cP;
     }

     // Starts the client loop in its own thread. The client loop processes the network traffic.
     // According to documentation (https://mosquitto.org/api/files/mosquitto-h.html#mosquitto_threaded_set):
     // "When using mosquitto_loop_start, this [mosquitto_threaded_set] is set automatically."
     mosquitto_loop_start(cP->mosq);

     // We block until the connect callback release the connection semaphore
     ts.tv_sec += timeout/1000;  // timeout is in miliseconds but tv_sec is in seconds
     cP->connectionCallbackCalled = false;
     sem_timedwait(&cP->connectionSem, &ts);
     if (!cP->connectionCallbackCalled)
     {
       alarmMgr.mqttConnectionError(endpoint, "connection timeout");

       // Not sure if this is actually needed (as the connection was never done in this case)
       // but let's use them just in case
       disconnect(cP->mosq, endpoint);
       cP->mosq = NULL;

       return cP;
     }

     // Check if the connection went ok (if not, e.g wrong user/pass, alarm is raised)
     // Note a value of 0 means success, check:
     // - For MQTT v5.0, look at section 3.2.2.2 Connect Reason code: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html
     // - For MQTT v3.1.1, look at section 3.2.2.3 Connect Return code: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
     if (cP->conectionResult)
     {
       alarmMgr.mqttConnectionError(endpoint, mosquitto_connack_string(cP->conectionResult));

       disconnect(cP->mosq, endpoint);
       cP->mosq = NULL;

       return cP;
     }

     // If it went ok, it is included in the connection map (and alarm is released)
     LM_T(LmtMqttNotif, ("MQTT successfull connection for %s", endpoint.c_str()));
     alarmMgr.mqttConnectionReset(endpoint);

     cP->lastTime = getCurrentTime();
     connections[endpoint] = cP;

     return cP;
  }
  else
  {
    // Already exists: update the time counter
    LM_T(LmtMqttNotif, ("Existing MQTT broker connection for %s", endpoint.c_str()));
    connections[endpoint]->lastTime = getCurrentTime();

    return connections[endpoint];
  }
}



/* ****************************************************************************
*
* MqttConnectionManager::sendMqttNotification -
*/
bool MqttConnectionManager::sendMqttNotification(const std::string& host, int port, const std::string& user, const std::string& passwd, const std::string& content, const std::string& topic, unsigned int qos, bool retain)
{
  std::string endpoint = getEndpoint(host, port);

  // A previous version of the implementation took the sem in getConnection(), but we
  // need to do it in sendMqttNotification to avoid the connection get removed by
  // the cleanup() method while is being used here (the probability is small, but
  // it could happen in theory)
  semTake();

  MqttConnection* cP = getConnection(host, port, user, passwd);
  mosquitto* mosq = cP->mosq;

  if (mosq == NULL)
  {
    // No need of log traces here: the getConnection() method would already print them.
    // In addition, note in this case (check getConnection()) cP was not added to the
    // connections std::map, so just freeing the memory allocated to cP pointer is enough
    delete cP;
    semGive();
    return false;
  }

  const char* msg = content.c_str();

  int id;

  bool retval;
  int resultCode = mosquitto_publish(mosq, &id, topic.c_str(), (int) strlen(msg), msg, qos, retain);
  if (resultCode != MOSQ_ERR_SUCCESS)
  {
    retval = false;
    alarmMgr.mqttConnectionError(endpoint, mosquitto_strerror(resultCode));
    // We destroy the connection in this case so a re-connection is forced next time
    // a MQTT notification is sent
    disconnect(cP->mosq, endpoint);
    connections.erase(endpoint);
    delete cP;
  }
  else
  {
    retval = true;
    LM_T(LmtMqttNotif, ("MQTT notification sent to %s:%d on topic %s with qos %d with id %d", host.c_str(), port, topic.c_str(), qos, id));
    alarmMgr.mqttConnectionReset(endpoint);
  }

  semGive();
  return retval;
}



/* ****************************************************************************
*
* MqttConnectionManager::cleanup -
*
* maxAge parameter is in seconds
*/
void MqttConnectionManager::cleanup(double maxAge)
{
  LM_T(LmtMqttNotif, ("Checking MQTT connections age"));

  semTake();

  std::vector<std::string> toErase;

  for (std::map<std::string, MqttConnection*>::iterator iter = connections.begin(); iter != connections.end(); ++iter)
  {
    std::string endpoint = iter->first;
    MqttConnection* cP   = iter->second;
    double age = getCurrentTime() - cP->lastTime;
    if (age > maxAge)
    {
      LM_T(LmtMqttNotif, ("MQTT connection %s too old (age: %f, maxAge: %f), removing it", endpoint.c_str(), age, maxAge));

      toErase.push_back(endpoint);
      disconnect(cP->mosq, endpoint);
      cP->mosq = NULL;

      delete cP;
    }
  }

  for (unsigned int ix = 0; ix < toErase.size(); ix++)
  {
    connections.erase(toErase[ix]);
  }

  semGive();
}
