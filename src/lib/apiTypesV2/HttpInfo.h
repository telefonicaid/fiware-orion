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
#include <map>

#include "rest/Verb.h"

#include "mongoDriver/BSONObj.h"
#include "parse/CompoundValueNode.h"
#include "apiTypesV2/CustomPayloadType.h"
#include "apiTypesV2/Entity.h"



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
  std::map<std::string, std::string>  qs;       // URI parameters
  std::map<std::string, std::string>  headers;
  std::string                         payload;  // either payload, json or ngsi is used (depending on payloadType)
  orion::CompoundValueNode*           json;     // either payload, json or ngsi is used (depending on payloadType)
  Entity                              ngsi;     // either payload, json or ngsi is used (depending on payloadType)
  CustomPayloadType                   payloadType;
  bool                                custom;
  bool                                includePayload;
  long long                           timeout;

  HttpInfo();

  std::string  toJson();
  void         fill(const orion::BSONObj& bo);
  void         fill(const HttpInfo& _httpInfo);
  void         release();
};
}

#endif  // SRC_LIB_APITYPESV2_HTTPINFO_H_
