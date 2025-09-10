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

#include "kafka/KafkaConnectionManager.h"

#include <iostream>
#include <mosquitto.h>
#include <string>
#include <vector>
#include <map>

#include "logMsg/logMsg.h"
#include "rest/HttpHeaders.h"
#include "common/globals.h"
#include "alarmMgr/alarmMgr.h"

#include <librdkafka/rdkafka.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

// FIXME #4666: Implement alarm feature for Kafka notifications. Have a look to parallel code in MQTT
// implementation and check which alarms are used there, trying to translate to equivalente methods
// in this Kafka connection manager

/* ****************************************************************************
*
* KAFKA CONTANTS-
*/
#define KAFKA_DESTROY_TIMEOUT_MS  3000
#define KAFKA_WAIT_INCREMENT_MS 100
#define KAFKA_FLUSH_TIMEOUT_MS 1000;

#define KAFKA_MAX_BUFFERING_MS_STR  "10"
#define KAFKA_MAX_MESSAGE_BYTES_STR "10000000"



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
  LM_T(LmtKafkaNotif, ("Initializing KAFKA library"));

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
  LM_T(LmtKafkaNotif, ("Teardown KAFKA connections"));

  for (std::map<std::string, KafkaConnection*>::iterator iter = connections.begin(); iter != connections.end(); ++iter)
  {
    std::string endpoint = iter->first;
    KafkaConnection* cP   = iter->second;

    disconnect(cP->producer, endpoint);
    cP->producer = NULL;

    delete cP;
  }
  connections.clear();

  // Global cleanup of librdkafka (wait up to KAFKA_DESTROY_TIMEOUT_MS seconds)
  int wait_time_ms = KAFKA_DESTROY_TIMEOUT_MS;
  while (rd_kafka_wait_destroyed(wait_time_ms) == -1)
  {
    LM_I(("Waiting for internal threads of librdkafka to finish..."));
    wait_time_ms -= KAFKA_WAIT_INCREMENT_MS;
    if (wait_time_ms <= 0)
    {
      break;
    }
  }

  LM_T(LmtKafkaNotif, ("Destruction of Kafka connections completed"));

}



/* ****************************************************************************
*
* KafkaConnectionManager::semInit -
*/
void KafkaConnectionManager::semInit(void)
{
  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_X(1, ("Fatal Error (error initializing 'kafka connection mgr' semaphore: %s)", strerror(errno)));
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
* KafkaConnectionManager::dispatchKafkaCallbacks -
*
* This function is invoked periodically to call rd_kafka_poll() so any pending
* callback can be called.
*
* Note this is different from MQTT, which doesn't need any poll to dispatch callbacks
* (in other words, callbacks are dispatched "automatically" by the library).
*
*/
void KafkaConnectionManager::dispatchKafkaCallbacks()
{
  // We copy the producers under the semaphore to minimize critical section time.
  std::vector<rd_kafka_t*> producers;
  producers.reserve(connections.size());

  semTake();
  for (std::map<std::string, KafkaConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
  {
    KafkaConnection* k = it->second;
    if (k && k->producer)
    {
      producers.push_back(k->producer);
    }
  }
  semGive();

  // We fire callbacks outside the critical section
  for (unsigned int ix = 0; ix < producers.size(); ++ix)
  {
    // 0 ms: does not block; if you call frequently, this is sufficient
    rd_kafka_poll(producers[ix], 0);
  }
}



/* ****************************************************************************
*
* KafkaConnectionManager::disconnect -
*/
void KafkaConnectionManager::disconnect(rd_kafka_t* producer, const std::string& endpoint)
{
  if (producer)
  {
    int kafka_flush_timeout_ms = KAFKA_FLUSH_TIMEOUT_MS;
    rd_kafka_resp_err_t err = rd_kafka_flush(producer, kafka_flush_timeout_ms); // Wait KAFKA_FLUSH_TIMEOUT_MS seconds to send pending messages
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
      LM_E(("Error al hacer flush de mensajes para %s: %s",
           endpoint.c_str(), rd_kafka_err2str(err)));
    }
    rd_kafka_destroy(producer);     // Free resources (equivalent to mosquitto_destroy)
  }
}



/* ****************************************************************************
*
* kafkaOnPublishCallback -
+
*/
void kafkaOnPublishCallback(rd_kafka_t* rk, const rd_kafka_message_t* msg, void* opaque)
{
  KafkaConnection* kConn = (KafkaConnection*) opaque;

  if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR)
  {
    LM_T(LmtKafkaNotif, ("Kafka notification successfully published at %s on topic %s",
                         kConn->endpoint.c_str(),
                         rd_kafka_topic_name(msg->rkt)));
  }
  else
  {
    LM_E(("Kafka delivery failed at %s: %s",
          kConn->endpoint.c_str(),
          rd_kafka_err2str(msg->err)));
  }
}



/* ****************************************************************************
*
* KafkaConnectionManager::getConnection -
*
*/
KafkaConnection* KafkaConnectionManager::getConnection(const std::string& endpoint)
{
  // Kafka uses endpoint "broker1:9092,broker2:9092"

  if (connections.find(endpoint) == connections.end())
  {
    LM_T(LmtKafkaNotif, ("Starting a new KAFKA broker connection for %s", endpoint.c_str()));

    KafkaConnection* kConn = new KafkaConnection();
    kConn->endpoint = endpoint;

    // Basic configuration (as in MQTT)
    rd_kafka_conf_t* conf = rd_kafka_conf_new();
    if (rd_kafka_conf_set(conf, "bootstrap.servers", endpoint.c_str(), NULL, 0) != RD_KAFKA_CONF_OK)
    {
      LM_E(("Runtime Error (Kafka config failed for %s)", endpoint.c_str()));
      delete kConn;
      return NULL;
    }
    rd_kafka_conf_set_opaque(conf, kConn);

    // Force UTF-8 and disable conversions
    rd_kafka_conf_set(conf, "message.encoding", "utf-8", NULL, 0);

    // Buffer configuration
    rd_kafka_conf_set(conf, "queue.buffering.max.ms", KAFKA_MAX_BUFFERING_MS_STR, NULL, 0);
    rd_kafka_conf_set(conf, "message.max.bytes", KAFKA_MAX_MESSAGE_BYTES_STR, NULL, 0);

    // Publication callback
    // Note that MQTT has a OnConnection callback and a OnPublish callback, but Kafka only has the former.
    // As a consequence, we don't need to synchronize connection with semaphores as in MQTT.
    rd_kafka_conf_set_dr_msg_cb(conf, kafkaOnPublishCallback);

    // Create producer
    kConn->producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, NULL, 0);
    if (!kConn->producer)
    {
      LM_E(("Runtime Error (Failed to create Kafka producer for %s)", endpoint.c_str()));
      delete kConn;
      return NULL;
    }

    LM_T(LmtKafkaNotif, ("KAFKA successful connection for %s", endpoint.c_str()));

    connections[endpoint] = kConn;
    return kConn;
  }

  LM_T(LmtKafkaNotif, ("Existing KAFKA broker connection for %s", endpoint.c_str()));
  connections[endpoint]->lastTime = getCurrentTime();
  return connections[endpoint];
}



/* ****************************************************************************
*
* KafkaConnectionManager::sendKafkaNotification -
*/
bool KafkaConnectionManager::sendKafkaNotification(
    const std::string& endpoint,
    const std::string& topic,
    const std::string& content,
    const std::string& subscriptionId,
    const std::string& tenant,
    const std::string& servicePath)

{

  semTake(); // Synchronization

  KafkaConnection* kConn = getConnection(endpoint);
  rd_kafka_t* producer = kConn ? kConn->producer : NULL;

  if (producer == NULL)
  {
    if (kConn)
    {
      delete kConn;
    }

    semGive();
    LM_E(("Kafka producer not available for %s", endpoint.c_str()));
    return false;
  }


  // by default will use the partitioner by key if you provide a key.
  // Automatic Partitioning:
  // When providing RD_KAFKA_V_KEY(), librdkafka automatically:
  // Calculates the hash of the key.
  // Selects a partition based on that hash (using the default murmur2 algorithm).
  // Messages with the same key → Same partition (guarantees order).


  rd_kafka_headers_t* headers = rd_kafka_headers_new(0); // Initially without headers

  if (!tenant.empty())
  {
    rd_kafka_header_add(headers, HTTP_FIWARE_SERVICE, -1, tenant.c_str(), tenant.size());
  }

  if (!servicePath.empty())
  {
    rd_kafka_header_add(headers, HTTP_FIWARE_SERVICEPATH, -1, servicePath.c_str(), servicePath.size());
  }

  bool retval = false;
  int resultCode = rd_kafka_producev(
            producer,
            RD_KAFKA_V_TOPIC(topic.c_str()),
            RD_KAFKA_V_KEY((void*)subscriptionId.data(), subscriptionId.size()),
            RD_KAFKA_V_VALUE(const_cast<char*>(content.data()), content.size()),
            RD_KAFKA_V_HEADERS(headers),
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            RD_KAFKA_V_END);

  if (resultCode != RD_KAFKA_RESP_ERR_NO_ERROR)
  {
    LM_E(("Kafka notification failed to %s on topic %s", endpoint.c_str(), topic.c_str()));
    disconnect(kConn->producer, endpoint);
    connections.erase(endpoint);
    delete kConn;
    retval = false;
  }
  else
  {
    // it may happend that the message gets published at this point, so the rd_kafka_poll() will dispatch its
    // callback in that case. Otherwise, that rd_kafka_poll() invokation will have no effect and the callback
    // be called in the perioricall invocation to dispatchKafkaCallbacks()
    kConn->lastTime = getCurrentTime();
    rd_kafka_poll(producer, 0);
    LM_T(LmtKafkaNotif, ("Kafka notification sent to %s on topic %s", endpoint.c_str(), topic.c_str()));
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
      toErase.push_back(endpoint);
      disconnect(kConn->producer, endpoint);
      LM_T(LmtKafkaNotif, ("Kafka connection %s too old (age: %f, maxAge: %f), removed.",
          endpoint.c_str(), age, maxAge));
      delete kConn;
    }
  }

  for (const auto& endpoint : toErase)
  {
    connections.erase(endpoint);
  }

  semGive();
}
