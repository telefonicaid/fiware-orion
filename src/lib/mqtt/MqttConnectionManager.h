#ifndef SRC_LIB_MQTT_MQTTCONNECTIONMANAGER_H_
#define SRC_LIB_MQTT_MQTTCONNECTIONMANAGER_H_

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
* Author: Fermin Galan
*/

#include <semaphore.h>
#include <sys/types.h>

#include <string>
#include <map>



/* ****************************************************************************
*
* MqttConnection -
*/
typedef struct MqttConnection
{
  struct mosquitto*  mosq;
  char*              userdata;
  double             lastTime;
} MqttConnection;



/* ****************************************************************************
*
* MqttConnectionManager -
*/
class MqttConnectionManager
{
 private:
  std::map<std::string, MqttConnection>  connections;
  sem_t                                  sem;

 public:
  MqttConnectionManager();

  int  init(void);
  void release(void);

  const char*  semGet(void);

  /*
  void connect(const std::string& url);
  void connect(const std::string& host, int port);
  void disconnect(const std::string& url);
  void disconnect(const std::string& host, int port);
  */
  int sendMqttNotification(const std::string& host, int port, const std::string& content, const std::string& topic, unsigned int qos);
  void cleanup(double maxAge, bool ignoreSems = false);

 private:
  int  semInit(void);
  void semTake(void);
  void semGive(void);

  MqttConnection getConnection(const std::string& host, int port);
};

#endif  // SRC_LIB_MQTT_MQTTCONNECTIONMANAGER_H_

