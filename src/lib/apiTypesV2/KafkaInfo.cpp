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
#include <string>

#include "logMsg/logMsg.h"
#include "common/JsonHelper.h"

#include "apiTypesV2/KafkaInfo.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/compoundResponses.h"

#include "mongoDriver/safeMongo.h"


namespace ngsiv2
{
/* ****************************************************************************
*
* KafkaInfo::KafkaInfo - 
*/
KafkaInfo::KafkaInfo() : qos(0), custom(false), json(NULL), payloadType(Text), includePayload(true), providedAuth(false)
{
}



/* ****************************************************************************
*
* KafkaInfo::toJson -
*/
std::string KafkaInfo::toJson()
{
  JsonObjectHelper jh;

  jh.addString("url", this->url);
  jh.addString("topic", this->topic);
  jh.addNumber("qos", (long long) this->qos);
  jh.addBool("retain", this->retain);

  if (providedAuth)
  {
    jh.addString("user", this->user);
    jh.addString("passwd", "*****");
  }

  if (custom)
  {
    switch (payloadType)
    {
    case Text:
      if (!this->includePayload)
      {
        jh.addNull("payload");
      }
      else if (!this->payload.empty())
      {
        jh.addString("payload", this->payload);
      }
      break;

    case Json:
      if (this->json != NULL)
      {
        jh.addRaw("json", this->json->toJson());
      }
      break;

    case Ngsi:
      jh.addRaw("ngsi", this->ngsi.toJson(NGSI_V2_NORMALIZED, true));
      break;
    }
  }

  return jh.str();
}



/* ****************************************************************************
*
* KafkaInfo::fill -
*/
void KafkaInfo::fill(const orion::BSONObj& bo)
{
  this->url    = bo.hasField(CSUB_REFERENCE)? getStringFieldF(bo, CSUB_REFERENCE) : "";
  this->topic  = bo.hasField(CSUB_KAFKATOPIC)? getStringFieldF(bo, CSUB_KAFKATOPIC) : "";
  // this->qos    = bo.hasField(CSUB_KAFKAQOS)?   getIntFieldF(bo, CSUB_MQTTQOS)      : 0;
  // this->retain = bo.hasField(CSUB_MQTTRETAIN)? getBoolFieldF(bo, CSUB_MQTTRETAIN) : false;
  this->custom = bo.hasField(CSUB_CUSTOM)?    getBoolFieldF(bo, CSUB_CUSTOM)      : false;

  // both user and passwd have to be used at the same time
  if ((bo.hasField(CSUB_USER)) && (bo.hasField(CSUB_PASSWD)))
  {
    this->user   = getStringFieldF(bo, CSUB_USER);
    this->passwd = getStringFieldF(bo, CSUB_PASSWD);
    this->providedAuth = true;
  }
  else
  {
    this->user   = "";
    this->passwd = "";
    this->providedAuth = false;
  }

  if (this->custom)
  {
    unsigned int n = 0;
    if (bo.hasField(CSUB_PAYLOAD)) n++;
    if (bo.hasField(CSUB_JSON))    n++;
    if (bo.hasField(CSUB_NGSI))    n++;
    if (n > 1)
    {
      LM_E(("Runtime Error (custom notification must not have more than one payload related field)"));
      return;
    }

    if (bo.hasField(CSUB_PAYLOAD))
    {
      payloadType = ngsiv2::CustomPayloadType::Text;

      if (getFieldF(bo, CSUB_PAYLOAD).isNull())
      {
        // We initialize also this->payload in this case, although its value is irrelevant
        this->payload = "";
        this->includePayload = false;
      }
      else
      {
        this->payload = getStringFieldF(bo, CSUB_PAYLOAD);
        this->includePayload = true;
      }
    }
    else
    {
      this->payload = "";
      this->includePayload = true;
    }

    if (bo.hasField(CSUB_JSON))
    {
      payloadType = ngsiv2::CustomPayloadType::Json;

      orion::BSONElement be = getFieldF(bo, CSUB_JSON);
      if (be.type() == orion::Object)
      {
        // this new memory is freed in KafkaInfo::release()
        this->json = new orion::CompoundValueNode(orion::ValueTypeObject);
        this->json->valueType = orion::ValueTypeObject;
        compoundObjectResponse(this->json, be);
      }
      else if (be.type() == orion::Array)
      {
        // this new memory is freed in KafkaInfo::release()
        this->json = new orion::CompoundValueNode(orion::ValueTypeVector);
        this->json->valueType = orion::ValueTypeVector;
        compoundVectorResponse(this->json, be);
      }
      else
      {
        LM_E(("Runtime Error (csub json field must be Object or Array but is %s)", orion::bsonType2String(be.type())));
      }
    }
    else
    {
      this->json = NULL;
    }

    if (bo.hasField(CSUB_NGSI))
    {
      payloadType = ngsiv2::CustomPayloadType::Ngsi;

      orion::BSONObj ngsiObj = getObjectFieldF(bo, CSUB_NGSI);
      if (ngsiObj.hasField(ENT_ENTITY_ID))
      {
        this->ngsi.entityId.id = getStringFieldF(ngsiObj, ENT_ENTITY_ID);
      }
      if (ngsiObj.hasField(ENT_ENTITY_TYPE))
      {
        this->ngsi.entityId.type = getStringFieldF(ngsiObj, ENT_ENTITY_TYPE);
      }
      if (ngsiObj.hasField(ENT_ATTRS))
      {
        this->ngsi.attributeVector.fill(getObjectFieldF(ngsiObj, ENT_ATTRS));
      }
    }
  }
}



/* ****************************************************************************
*
* KafkaInfo::fill -
*/
void KafkaInfo::fill(const KafkaInfo& _KafkaInfo)
{
  this->url            = _KafkaInfo.url;
  this->topic          = _KafkaInfo.topic;
  this->qos            = _KafkaInfo.qos;
  this->retain         = _KafkaInfo.retain;
  this->custom         = _KafkaInfo.custom;
  this->payload        = _KafkaInfo.payload;
  this->payloadType    = _KafkaInfo.payloadType;
  this->includePayload = _KafkaInfo.includePayload;
  this->providedAuth   = _KafkaInfo.providedAuth;
  this->user           = _KafkaInfo.user;
  this->passwd         = _KafkaInfo.passwd;

  this->json = _KafkaInfo.json == NULL ? NULL : _KafkaInfo.json->clone();

  this->ngsi.fill(_KafkaInfo.ngsi, false, true);  // clone compounds enabled
}



/* ****************************************************************************
*
* KafkaInfo::release -
*/
void KafkaInfo::release()
{
  if (json != NULL)
  {
    // This will cause the orion::CompoundValueNode destructor to be called, which
    // recursively frees all memory
    delete json;
    json = NULL;
  }
  ngsi.release();
}
}
