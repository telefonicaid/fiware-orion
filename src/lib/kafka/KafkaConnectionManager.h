#ifndef SRC_LIB_KAFKA_KAFKACONNECTIONMANAGER_H_
#define SRC_LIB_KAFKA_KAFKACONNECTIONMANAGER_H_

/*
*
* Copyright 2025 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Oriana Romero (based in MQTT implementation from Burak Karaboga)
*/

#include <semaphore.h>
#include <sys/types.h>

#include <string>
#include <map>
#include <rdkafka.h>


/* ****************************************************************************
*
* KafkaConnection -
*/
typedef struct KafkaConnection
{
 rd_kafka_t*        producer;       // similar to mosquitto*
 std::string        endpoint;       // "broker1:9092,broker2:9092"
 double             lastTime;       // Last activity timestamp
 int                connectionResult; //  Error code (rd_kafka_resp_err_t)
 std::string        key;
 std::string        securityProtocol;
 std::string        saslMechanism;
 std::string        user;
} KafkaConnection;


/* ****************************************************************************
*
* KafkaConnectionManager -
*/
class KafkaConnectionManager
{
private:
 std::map<std::string, KafkaConnection*>  connections;  // Map by endpoint
 long                                    timeout;       // Timeout in ms
 sem_t                                   sem;           // Global traffic light

public:
 KafkaConnectionManager();
 void init(long _timeout);
 void teardown(void);

 const char*  semGet(void);

 bool sendKafkaNotification(
  const std::string& endpoint,
  const std::string& topic,
  const std::string& content,
  const std::string& subscriptionId,
  const std::string& tenant,
  const std::string& servicePath,
  const std::map<std::string, std::string>& customHeaders,
  const std::string& user,
  const std::string& passwd,
  const std::string& saslMechanism,
  const std::string& securityProtocol
 );

 void cleanup(double maxAge);  // Cleaning inactive connections
 void dispatchKafkaCallbacks();

private:
 void disconnect(rd_kafka_t* producer, const std::string& endpoint);
 void semInit(void);
 void semTake(void);
 void semGive(void);
 KafkaConnection* getConnection(const std::string& endpoint,
                                const std::string& user,
                                const std::string& passwd,
                                const std::string& saslMechanism,
                                const std::string& securityProtocol);
};

#endif  // SRC_LIB_KAFKA_KAFKACONNECTIONMANAGER_H_

