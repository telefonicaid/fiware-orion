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
#include <rdkafka.h>


/* ****************************************************************************
*
* KafkaConnection -
*/
typedef struct KafkaConnection
{
 rd_kafka_t*        producer;       // Similar a mosquitto*
 std::string        endpoint;       // "broker1:9092,broker2:9092"
 double             lastTime;       // Timestamp de última actividad
 sem_t              connectionSem;  // Sincronización (como en MQTT)
 int                connectionResult; // Código de error (rd_kafka_resp_err_t)
 bool               connectionCallbackCalled; // Para callbacks
} KafkaConnection;


/* ****************************************************************************
*
* KafkaConnectionManager -
*/
class KafkaConnectionManager
{
private:
 std::map<std::string, KafkaConnection*>  connections;  // Mapa por endpoint
 long                                    timeout;       // Timeout en ms
 sem_t                                   sem;           // Semaforo global

public:
 KafkaConnectionManager();
 void init(long _timeout);
 void teardown(void);

 const char*  semGet(void);

 bool sendKafkaNotification(
   const std::string& brokers,
   const std::string& topic,
   const std::string& message,
   const std::string& tenant,
   const std::string& servicePath,
   int partition = RD_KAFKA_PARTITION_UA  // Partitioning opcional

 );

 void cleanup(double maxAge);  // Limpieza de conexiones inactivas

private:
 void disconnect(rd_kafka_t* producer, const std::string& endpoint);
 void semInit(void);
 void semTake(void);
 void semGive(void);
 KafkaConnection* getConnection(const std::string& brokers);
};

#endif  // SRC_LIB_KAFKA_KAFKACONNECTIONMANAGER_H_

