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

#include "kafka/KafkaConnectionManager.h"

#include <mosquitto.h>
#include <string>
#include <vector>
#include <map>

#include "logMsg/logMsg.h"
#include "common/globals.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* KAFKA_DEFAULT_KEEPALIVE -
*/
#define KAFKA_DEFAULT_KEEPALIVE 60



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



// /* ****************************************************************************
// *
// * mqttOnConnectCallback -
// */
// void mqttOnConnectCallback(struct mosquitto* mosq, void *userdata, int rc)
// {
//   KafkaConnection* cP = (KafkaConnection*) userdata;
//
//   // To be used in getConnetion()
//   cP->conectionResult = rc;
//
//   // Signal that callback has been called
//   cP->connectionCallbackCalled = true;
//
//   // This allows the code waiting in getConnection() to continue
//   sem_post(&cP->connectionSem);
// }



// /* ****************************************************************************
// *
// * mqttOnPublishCallback -*/
// void mqttOnPublishCallback(struct mosquitto *mosq, void *userdata, int mid)
// {
//   KafkaConnection* cP = (KafkaConnection*) userdata;
//
//   // mid could be used to correlate. By the moment we only print it in log traces at DEBUG log level
//   // Note this trace use N/A in corrid= and transid=
//   LM_T(LmtMqttNotif, ("KAFKA notification successfully published at %s with id %d", cP->endpoint.c_str(), mid));
// }



/* ****************************************************************************
*
* KafkaConnectionManager::KafkaConnectionManager -
*/
KafkaConnectionManager::KafkaConnectionManager(void)
{
}



/* ****************************************************************************
*
* KafkaConnectionManager::init -
*/
void KafkaConnectionManager::init(long _timeout)
{
  LM_T(LmtMqttNotif, ("Initializing KAFKA library"));
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
* KafkaConnectionManager::teardown -
*/
void KafkaConnectionManager::teardown(void)
{
  LM_T(LmtMqttNotif, ("Teardown KAFKA connections"));

  for (std::map<std::string, KafkaConnection*>::iterator iter = connections.begin(); iter != connections.end(); ++iter)
  {
    std::string endpoint = iter->first;
    KafkaConnection* cP   = iter->second;

    disconnect(cP->producer, endpoint);
    cP->producer = NULL;

    delete cP;
  }
  connections.clear();

  // 5. Limpieza global de librdkafka (esperar hasta 3 segundos)
  int wait_time_ms = 3000;
  while (rd_kafka_wait_destroyed(wait_time_ms) == -1)
  {
    LM_I(("Esperando finalización de threads internos de librdkafka..."));
    wait_time_ms -= 100;
    if (wait_time_ms <= 0) break;
  }

  LM_T(LmtKafkaNotif, ("Destrucción de conexiones Kafka completada"));

}



/* ****************************************************************************
*
* KafkaConnectionManager::semInit -
*/
void KafkaConnectionManager::semInit(void)
{
  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_X(1, ("Fatal Error (error initializing 'mqtt connection mgr' semaphore: %s)", strerror(errno)));
  }
}



/* ****************************************************************************
*
* KafkaConnectionManager::semGet -
*/
const char* KafkaConnectionManager::semGet(void)
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
* KafkaConnectionManager::semTake -
*/
void KafkaConnectionManager::semTake(void)
{
  sem_wait(&sem);
}



/* ****************************************************************************
*
* KafkaConnectionManager::semGive -
*/
void KafkaConnectionManager::semGive(void)
{
  sem_post(&sem);
}



/* ****************************************************************************
*
* KafkaConnectionManager::disconnect -
*/
void KafkaConnectionManager::disconnect(rd_kafka_t* producer, const std::string& endpoint)
{
  if (producer)
  {
    rd_kafka_resp_err_t err = rd_kafka_flush(producer, 1000); // Espera 1s para enviar mensajes pendientes
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
      LM_E(("Error al hacer flush de mensajes para %s: %s",
           endpoint.c_str(), rd_kafka_err2str(err)));
    }
    rd_kafka_destroy(producer);     // Libera recursos (equivalente a mosquitto_destroy)
  }
  // alarmMgr.kafkaConnectionError(endpoint, "disconnected");
}

/* ****************************************************************************
*
* KafkaConnectionManager::getConnection -
*/
KafkaConnection* KafkaConnectionManager::getConnection(const std::string& brokers)
{
  std::string endpoint = brokers; // Kafka usa "broker1:9092,broker2:9092"

  if (connections.find(endpoint) == connections.end())
  {
    KafkaConnection* kConn = new KafkaConnection();
    kConn->endpoint = endpoint;

    // Configuración básica (como en MQTT)
    rd_kafka_conf_t* conf = rd_kafka_conf_new();
    if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers.c_str(), nullptr, 0) != RD_KAFKA_CONF_OK)
    {
      LM_E(("Runtime Error (Kafka config failed for %s)", endpoint.c_str()));
      delete kConn;
      return nullptr;
    }

    // Callback de conexión (similar a MQTT)
    rd_kafka_conf_set_dr_msg_cb(conf, [](rd_kafka_t* rk, const rd_kafka_message_t* msg, void* opaque) {
      KafkaConnection* kConn = static_cast<KafkaConnection*>(opaque);
      kConn->connectionResult = msg->err;
      kConn->connectionCallbackCalled = true;
      sem_post(&kConn->connectionSem);
    });

    // Crear producer (como mosquitto_new en MQTT)
    kConn->producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, nullptr, 0);
    if (!kConn->producer)
    {
      LM_E(("Runtime Error (Failed to create Kafka producer for %s)", endpoint.c_str()));
      delete kConn;
      return nullptr;
    }

    // Esperar conexión (como sem_timedwait en MQTT)
    sem_init(&kConn->connectionSem, 0, 0);
    kConn->connectionCallbackCalled = false;

    // En Kafka, la conexión es lazy, pero podemos forzar un test
    rd_kafka_poll(kConn->producer, 0);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout / 1000;
    sem_timedwait(&kConn->connectionSem, &ts);

    // if (!kConn->connectionCallbackCalled || kConn->connectionResult != RD_KAFKA_RESP_ERR_NO_ERROR)
    // {
    //   LM_E(("Kafka connection error for %s: %s", endpoint.c_str(), rd_kafka_err2str(kConn->connectionResult)));
    //   disconnect(kConn->producer, endpoint);
    //   delete kConn;
    //   return nullptr;
    // }

    connections[endpoint] = kConn;
    return kConn;
  }
  else
  {
    // Conexión existente (actualizar timestamp)
    connections[endpoint]->lastTime = getCurrentTime();
    return connections[endpoint];
  }
}



/* ****************************************************************************
*
* KafkaConnectionManager::sendMqttNotification -
*/
bool KafkaConnectionManager::sendKafkaNotification(
    const std::string& brokers,
    const std::string& topic,
    const std::string& content,
    int partition /* = RD_KAFKA_PARTITION_UA */)
{
  std::string endpoint = brokers; // Kafka usa "broker1:9092,broker2:9092"

  semTake(); // Sincronización (igual que en MQTT)

  KafkaConnection* kConn = getConnection(brokers);
  rd_kafka_t* producer = kConn ? kConn->producer : nullptr;

  if (producer == nullptr)
  {
    if (kConn) delete kConn; // Limpieza si falló getConnection()
    semGive();
    LM_E(("Kafka producer not available for %s", endpoint.c_str()));
    return false;
  }

  bool retval = false;
  int resultCode = rd_kafka_produce(
      rd_kafka_topic_new(producer, topic.c_str(), nullptr),
      partition,
      RD_KAFKA_MSG_F_COPY,
      const_cast<char*>(content.data()), content.size(),
      nullptr, 0,
      nullptr
  );

  if (resultCode != RD_KAFKA_RESP_ERR_NO_ERROR)
  {
    alarmMgr.mqttConnectionError(endpoint, rd_kafka_err2str(rd_kafka_last_error()));
    disconnect(kConn->producer, endpoint);
    connections.erase(endpoint);
    delete kConn;
    retval = false;
  }
  else
  {
    kConn->lastTime = getCurrentTime();
    rd_kafka_poll(producer, 0); // Procesar eventos
    LM_T(LmtKafkaNotif, ("Kafka notification sent to %s on topic %s", brokers.c_str(), topic.c_str()));
    alarmMgr.mqttConnectionReset(endpoint);
    retval = true;
  }

  semGive();
  return retval;
}



/* ****************************************************************************
*
* KafkaConnectionManager::cleanup -
*
* maxAge parameter is in seconds
*/
void KafkaConnectionManager::cleanup(double maxAge)
{
  LM_T(LmtKafkaNotif, ("Checking Kafka connections age"));

  semTake();

  std::vector<std::string> toErase;

  for (auto iter = connections.begin(); iter != connections.end(); ++iter)
  {
    std::string endpoint = iter->first;
    KafkaConnection* kConn = iter->second;
    double age = getCurrentTime() - kConn->lastTime;

    if (age > maxAge)
    {
      LM_T(LmtKafkaNotif, ("Kafka connection %s too old (age: %f, maxAge: %f), removing it",
          endpoint.c_str(), age, maxAge));

      toErase.push_back(endpoint);
      disconnect(kConn->producer, endpoint);
      delete kConn;
    }
  }

  for (const auto& endpoint : toErase)
  {
    connections.erase(endpoint);
  }

  semGive();
}
