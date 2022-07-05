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

#include "apiTypesV2/MqttInfo.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/compoundResponses.h"

#include "mongoDriver/safeMongo.h"


namespace ngsiv2
{
/* ****************************************************************************
*
* MqttInfo::MqttInfo - 
*/
MqttInfo::MqttInfo() : qos(0), custom(false), json(NULL), includePayload(true), providedAuth(false)
{
}



/* ****************************************************************************
*
* MqttInfo::MqttInfo - 
*/
MqttInfo::MqttInfo(const std::string& _url) : url(_url), qos(0), custom(false), json(NULL), includePayload(true), providedAuth(false)
{
}



/* ****************************************************************************
*
* MqttInfo::toJson -
*/
std::string MqttInfo::toJson()
{
  JsonObjectHelper jh;

  jh.addString("url", this->url);
  jh.addString("topic", this->topic);
  jh.addNumber("qos", (long long) this->qos);

  if (providedAuth)
  {
    jh.addString("user", this->user);
    jh.addString("passwd", "*****");
  }

  if (custom)
  {
    if (!this->includePayload)
    {
      jh.addNull("payload");
    }
    else if (!this->payload.empty())
    {
      jh.addString("payload", this->payload);
    }

    if (this->json != NULL)
    {
      jh.addRaw("json", this->json->toJson());
    }
  }

  return jh.str();
}



/* ****************************************************************************
*
* MqttInfo::fill -
*/
void MqttInfo::fill(const orion::BSONObj& bo)
{
  this->url    = bo.hasField(CSUB_REFERENCE)? getStringFieldF(bo, CSUB_REFERENCE) : "";
  this->topic  = bo.hasField(CSUB_MQTTTOPIC)? getStringFieldF(bo, CSUB_MQTTTOPIC) : "";
  this->qos    = bo.hasField(CSUB_MQTTQOS)?   getIntFieldF(bo, CSUB_MQTTQOS)      : 0;
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
    if (bo.hasField(CSUB_PAYLOAD))
    {
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
      orion::BSONElement be = getFieldF(bo, CSUB_JSON);
      if (be.type() == orion::Object)
      {
        // this new memory is freed in MqttInfo::release()
        this->json = new orion::CompoundValueNode(orion::ValueTypeObject);
        this->json->valueType = orion::ValueTypeObject;
        compoundObjectResponse(this->json, be);
      }
      else if (be.type() == orion::Array)
      {
        // this new memory is freed in MqttInfo::release()
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
  }
}



/* ****************************************************************************
*
* MqttInfo::release -
*/
void MqttInfo::release()
{
  if (json != NULL)
  {
    // This will cause the orion::CompoundValueNode destructor to be called, which
    // recursively frees all memory
    delete json;
    json = NULL;
  }
}
}
