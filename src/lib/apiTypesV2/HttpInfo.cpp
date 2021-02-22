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

#include "mongo/client/dbclient.h"
#include "logMsg/logMsg.h"

#include "common/JsonHelper.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"
#include "orionld/mqtt/mqttParse.h"                            // mqttParse
#include "orionld/common/orionldState.h"                       // orionldState

#include "apiTypesV2/HttpInfo.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONObj;
using mongo::BSONElement;



namespace ngsiv2
{
/* ****************************************************************************
*
* HttpInfo::HttpInfo -
*/
#ifdef ORIONLD
HttpInfo::HttpInfo() : verb(NOVERB), custom(false), mimeType(DEFAULT_MIMETYPE)
#else
HttpInfo::HttpInfo() : verb(NOVERB), custom(false)
#endif
{
#ifdef ORIONLD
  bzero(&mqtt, sizeof(mqtt));
#endif
}



/* ****************************************************************************
*
* HttpInfo::HttpInfo -
*/
#ifdef ORIONLD
HttpInfo::HttpInfo(const std::string& _url) : url(_url), verb(NOVERB), custom(false), mimeType(DEFAULT_MIMETYPE)
#else
HttpInfo::HttpInfo(const std::string& _url) : url(_url), verb(NOVERB), custom(false)
#endif
{
#ifdef ORIONLD
  bzero(&mqtt, sizeof(mqtt));
#endif
}



/* ****************************************************************************
*
* HttpInfo::toJson -
*/
std::string HttpInfo::toJson()
{
  JsonHelper jh;

  jh.addString("url", this->url);

  if (custom)
  {
    if (this->payload != "")
    {
      jh.addString("payload", this->payload);
    }

    if (this->verb != NOVERB)
    {
      jh.addString("method", verbName(this->verb));
    }

    if (qs.size() != 0)
    {
      jh.addRaw("qs", objectToJson(qs));
    }

    if (headers.size() != 0)
    {
      jh.addRaw("headers", objectToJson(headers));
    }
  }

  return jh.str();
}



/* ****************************************************************************
*
* HttpInfo::fill -
*/
void HttpInfo::fill(const BSONObj& bo)
{
  this->url    = bo.hasField(CSUB_REFERENCE)? getStringFieldF(bo, CSUB_REFERENCE) : "";
  this->custom = bo.hasField(CSUB_CUSTOM)?    getBoolFieldF(bo,   CSUB_CUSTOM)    : false;

  bool mqtt = (strncmp(url.c_str(), "mqtt", 4) == 0);

#ifdef ORIONLD
  std::string mimeTypeString;

  mimeTypeString = bo.hasField(CSUB_MIMETYPE)? getStringFieldF(bo, CSUB_MIMETYPE) : "application/json";  // Default
  this->mimeType = longStringToMimeType(mimeTypeString);

  if (bo.hasField("notifierInfo"))
  {
    BSONObj ni = getObjectFieldF(bo, "notifierInfo");

    for (BSONObj::iterator iter = ni.begin(); iter.more();)
    {
      mongo::BSONElement be = iter.next();

      const char*  key   = be.fieldName();
      const char*  value = be.String().c_str();
      KeyValue*    kvP   = (KeyValue*) kaAlloc(&orionldState.kalloc, sizeof(KeyValue));  // FIXME: what about initial cache fill?

      strncpy(kvP->key,   key,   sizeof(kvP->key));
      strncpy(kvP->value, value, sizeof(kvP->value));

      notifierInfo.push_back(kvP);
    }
  }
#endif

  if (this->custom)
  {
    this->payload  = bo.hasField(CSUB_PAYLOAD)? getStringFieldF(bo, CSUB_PAYLOAD) : "";

    if (bo.hasField(CSUB_METHOD))
    {
      this->verb = str2Verb(getStringFieldF(bo, CSUB_METHOD));
    }

    // qs
    if (bo.hasField(CSUB_QS))
    {
      BSONObj qs = getObjectFieldF(bo, CSUB_QS);

      for (BSONObj::iterator i = qs.begin(); i.more();)
      {
        BSONElement e = i.next();

        this->qs[e.fieldName()] = e.String();
      }
    }
  }

  if (this->custom || mqtt)
  {
    // headers
    if (bo.hasField(CSUB_HEADERS))
    {
      BSONObj headers = getObjectFieldF(bo, CSUB_HEADERS);

      for (BSONObj::iterator i = headers.begin(); i.more();)
      {
        BSONElement e = i.next();

        this->headers[e.fieldName()] = e.String();
      }
    }
  }

  if (mqtt)
  {
    char*           url           = strdup(this->url.c_str());
    bool            mqtts         = false;
    char*           mqttUser      = NULL;
    char*           mqttPassword  = NULL;
    char*           mqttHost      = NULL;
    unsigned short  mqttPort      = 0;
    char*           mqttTopic     = NULL;
    char*           detail        = NULL;

    if (mqttParse(url, &mqtts, &mqttUser, &mqttPassword, &mqttHost, &mqttPort, &mqttTopic, &detail) == false)
    {
      free(url);
      LM_E(("Internal Error (unable to parse mqtt URL)"));
      return;
    }

    if (mqttUser     != NULL) strncpy(this->mqtt.username, mqttUser,     sizeof(this->mqtt.username));
    if (mqttPassword != NULL) strncpy(this->mqtt.password, mqttPassword, sizeof(this->mqtt.password));
    if (mqttHost     != NULL) strncpy(this->mqtt.host,     mqttHost,     sizeof(this->mqtt.host));
    if (mqttTopic    != NULL) strncpy(this->mqtt.topic,    mqttTopic,    sizeof(this->mqtt.topic));

    this->mqtt.mqtts = mqtts;
    this->mqtt.port  = mqttPort;

    free(url);
  }
}

}  // namespace ngsiv2
