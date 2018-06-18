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
#include <map>

#include "mongo/client/dbclient.h"
#include "logMsg/logMsg.h"

#include "common/JsonHelper.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"
#include "apiTypesV2/MqttInfo.h"



namespace ngsiv2
{

/* ****************************************************************************
*
* MqttInfo::MqttInfo - 
*/
MqttInfo::MqttInfo() : topic(EMPTY_TOPIC), custom(false)
{
}



/* ****************************************************************************
*
* MqttInfo::MqttInfo - 
*/
MqttInfo::MqttInfo(const std::string& _topic) : topic(_topic), custom(false)
{
}



/* ****************************************************************************
*
* MqttInfo::toJson -
*/
std::string MqttInfo::toJson()
{
  JsonHelper jh;

  jh.addString("topic", this->topic);

  if (custom)
  {
    // FIXME: Planned for second MQTT PR
  }

  return jh.str();
}



/* ****************************************************************************
*
* MqttInfo::fill -
*/
void MqttInfo::fill(const BSONObj& bo)
{
  this->topic  = bo.hasField(CSUB_REFERENCE)? getStringFieldF(bo, CSUB_REFERENCE) : DEFAULT_MQTT_TOPIC;
  this->custom = bo.hasField(CSUB_CUSTOM)?    getBoolFieldF(bo,   CSUB_CUSTOM)    : false;

  if (this->custom)
  {
    // FIXME: Planned for second MQTT PR
  }
}

}
