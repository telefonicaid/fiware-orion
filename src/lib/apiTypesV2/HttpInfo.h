#ifndef SRC_LIB_APITYPESV2_HTTPINFO_H_
#define SRC_LIB_APITYPESV2_HTTPINFO_H_

/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Orion dev team
*/
#include <string>
#include <vector>
#include <map>

#include "mongo/client/dbclient.h"
#include "rest/Verb.h"

#ifdef ORIONLD
#include "common/MimeType.h"              // MimeType
#endif



/* ****************************************************************************
*
* KeyValue - FIXME: move!
*/
typedef struct KeyValue
{
  char key[64];
  char value[64];
} KeyValue;



/* ****************************************************************************
*
* MqttInfo - FIXME: move!
*/
typedef struct MqttInfo
{
  bool            mqtts;
  char            host[64];
  unsigned short  port;
  char            topic[128];
  char            username[128];
  char            password[128];
  char            version[16];
  int             qos;
} MqttInfo;



namespace ngsiv2
{
/* ****************************************************************************
*
* HttpInfo - 
*/
struct HttpInfo
{
  std::string                         url;
  Verb                                verb;
  std::map<std::string, std::string>  qs;      // URI parameters
  std::map<std::string, std::string>  headers;
  std::string                         payload;
  bool                                custom;
#ifdef ORIONLD
  MimeType                            mimeType;
  MqttInfo                            mqtt;
  std::vector<KeyValue*>              notifierInfo;
#endif
  HttpInfo();
  explicit HttpInfo(const std::string& _url);

  std::string  toJson();
  void         fill(const mongo::BSONObj& bo);
};
}

#endif  // SRC_LIB_APITYPESV2_HTTPINFO_H_
