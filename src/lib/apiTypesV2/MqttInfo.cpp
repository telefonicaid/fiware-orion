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

#include "mongoDriver/safeMongo.h"


namespace ngsiv2
{
/* ****************************************************************************
*
* MqttInfo::MqttInfo - 
*/
MqttInfo::MqttInfo() : qos(0), custom(false), includePayload(true)
{
}



/* ****************************************************************************
*
* MqttInfo::MqttInfo - 
*/
MqttInfo::MqttInfo(const std::string& _url) : url(_url), qos(0), custom(false), includePayload(true)
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
  }
}
}
