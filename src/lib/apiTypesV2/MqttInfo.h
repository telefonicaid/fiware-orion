#ifndef SRC_LIB_APITYPESV2_MQTTINFO_H_
#define SRC_LIB_APITYPESV2_MQTTINFO_H_

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

#include "mongoDriver/BSONObj.h"

namespace ngsiv2
{
/* ****************************************************************************
*
* MqttInfo - 
*/
struct MqttInfo
{
  std::string   endpoint;
  std::string   topic;
  unsigned int  qos;  // 0, 1 or 2

  bool          custom;
  std::string   payload;
  bool          includePayload;

  MqttInfo();
  explicit MqttInfo(const std::string& _endpoint);

  std::string  toJson();
  void         fill(const orion::BSONObj& bo);
};
}

#endif  // SRC_LIB_APITYPESV2_MQTTINFO_H_
