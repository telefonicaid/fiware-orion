#ifndef SRC_LIB_KAFKA_KAFKACONNECTIONMANAGER_H_
#define SRC_LIB_KAFKA_KAFKACONNECTIONMANAGER_H_

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
* KafkaConnection -
*/
typedef struct KafkaConnection
{
  struct mosquitto*  mosq;
  std::string        endpoint;
  double             lastTime;
  sem_t              connectionSem;
  int                conectionResult;
  bool               connectionCallbackCalled;
} KafkaConnection;



/* ****************************************************************************
*
* KafkaConnectionManager -
*/
class KafkaConnectionManager
{
 private:
  std::map<std::string, KafkaConnection*>  connections;
  long                                    timeout;
  sem_t                                   sem;

 public:
  KafkaConnectionManager();

  void init(long _timeout);
  void teardown(void);

  const char*  semGet(void);

  bool sendMqttNotification(const std::string& host, int port, const std::string& user, const std::string& passwd, const std::string& content, const std::string& topic, unsigned int qos, bool retain);
  void cleanup(double maxAge);

 private:
  void disconnect(struct mosquitto*  mosq, const std::string& endpoint);
  void semInit(void);
  void semTake(void);
  void semGive(void);

  KafkaConnection* getConnection(const std::string& host, int port, const std::string& user, const std::string& passwd);
};

#endif  // SRC_LIB_KAFKA_KAFKACONNECTIONMANAGER_H_

